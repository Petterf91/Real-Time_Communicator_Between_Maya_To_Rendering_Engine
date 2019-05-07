#include "maya_includes.h"
#include <maya/MItDependencyGraph.h>
#include <maya/MTimer.h>
#include <maya/MDagPathArray.h>
#include <iostream>
#include <string>
#include <algorithm>
#include <vector>
#include <queue>
#include <ComLib.h>

using namespace std;
MCallbackIdArray callbackIdArray;
MStatus status = MS::kSuccess;
size_t memorySize;

ComLib producer("sharedMem", (size_t)10, ComLib::TYPE::PRODUCER);

char* buff = (char*)malloc(1024 * 1024 * 10);

enum msgType {
	TRANSFORM,
	VERTICES,
	MATERIAL, 
	CAMERA,
	LIGHTPOS
};

enum camType {
	PERSPECTIVE,
	ORTHOGRAPHIC
};

struct IdTracker {
	vector<int> id;
	vector<bool> newMesh;
	vector<MString> name;
	vector<MString> texture;
};

struct OurTransform {
	int messagetype = TRANSFORM;
	int id;
	float translation[3];
	float rotation[3];
	float scale[3];
};

struct Vertex {
	float x, y, z;
	float nx, ny, nz;
	float u, v;
};

struct OurVertices {
	int messagetype = VERTICES;
	int id;
	int nrOfVertices;
	int nrOfIndices;
};

struct OurLight {
	int messagetype = LIGHTPOS;
	float position[3];
};

struct OurMaterial {
	int messagetype = MATERIAL;
	int id;
	float color[3];
	char filePath[200];
};

struct OurCamera {
	int messageType = CAMERA;
	int type;
	float eye[3];
	float target[3];
	float up[3];
	float viewDir[3];
	float orthoDist;
	float aspectRatio;
};

IdTracker idMesh;
IdTracker idTransform;
int newMeshCheck;

//Check funktions are used to keep track of object ids
int idCheckTransform(MFnTransform &fnTransform)
{
	bool check = false;

	for (int i = 0; i < idTransform.name.size(); i++)
	{
		if (fnTransform.name() == idTransform.name[i])
		{
			return idTransform.id[i];
			check = true;
		}
	}

	if (!check)
	{
		idTransform.name.push_back(fnTransform.name());
		idTransform.id.push_back(idTransform.id.size());
		return idTransform.id[idTransform.id.size()-1];
	}
}

int idCheckVertex(MFnMesh &fnMesh)
{
	bool check = false;

	for (int i = 0; i < idMesh.name.size(); i++)
	{
		if (fnMesh.name() == idMesh.name[i])
		{
			int temp = idMesh.id[i];
			int x = 0;
			return idMesh.id[i];
			check = true;
		}
	}

	if (!check)
	{
		idMesh.name.push_back(fnMesh.name());
		idMesh.id.push_back(idMesh.id.size());
		return idMesh.id[idMesh.id.size()-1];
	}
}

bool checkTextureName(MString textureName, int meshId)
{
	if (idMesh.texture.size() > meshId)
	{
		if (textureName == idMesh.texture[meshId])
		{
			return false;
		}
		else
		{
			idMesh.texture[meshId] = textureName;
			return true;
		}
	}
	else
	{
		idMesh.texture.push_back(textureName);
		return true;
	}
}

bool checkMesh(MFnMesh &fnMesh)
{
	for (int i = 0; i < idMesh.name.size(); i++)
	{
		if (fnMesh.name() == idMesh.name[i])
		{
			if (idMesh.newMesh[i])
			{
				return false;
			}
			else
			{
				idMesh.newMesh[i] = true;
				return true;
			}
		}
	}

	idMesh.newMesh.push_back(false);
	return true;
}

void nodeAttributeChanged(MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void* x)
{
	if (msg & MNodeMessage::kAttributeSet)
	{
		if (plug.node().hasFn(MFn::kTransform))
		{
			MDagPath path;
			MFnDagNode(plug.node()).getPath(path);
			MFnTransform fnTransform(path);

			if (fnTransform.child(0).hasFn(MFn::kMesh))
			{
				MVector translationGlobal;
				translationGlobal = fnTransform.getTranslation(MSpace::kWorld);

				MEulerRotation rotationLocal;
				fnTransform.getRotation(rotationLocal);

				double scaleLocal[3] = { 0 };
				fnTransform.getScale(scaleLocal);

				OurTransform newTransform;

				newTransform.id = idCheckTransform(fnTransform);

				newTransform.translation[0] = (float)translationGlobal.x;
				newTransform.translation[1] = (float)translationGlobal.y;
				newTransform.translation[2] = (float)translationGlobal.z;
				newTransform.rotation[0] = (float)rotationLocal.x;
				newTransform.rotation[1] = (float)rotationLocal.y;
				newTransform.rotation[2] = (float)rotationLocal.z;
				newTransform.scale[0] = (float)scaleLocal[0];
				newTransform.scale[1] = (float)scaleLocal[1];
				newTransform.scale[2] = (float)scaleLocal[2];

				producer.write(&newTransform, sizeof(newTransform));
			}
			else if (fnTransform.child(0).hasFn(MFn::kLight))
			{
				MVector translationGlobal;
				translationGlobal = fnTransform.getTranslation(MSpace::kWorld);
					
				OurLight newLight;
				newLight.position[0] = translationGlobal.x;
				newLight.position[1] = translationGlobal.y;
				newLight.position[2] = translationGlobal.z;

				producer.write(&newLight, sizeof(newLight));
			}
		}
		else if (plug.node().hasFn(MFn::kMesh))
		{
			MFnMesh fnMesh(plug.node(), &status);
			OurVertices newVertices;

			if (status == MS::kSuccess)
			{
				if (checkMesh(fnMesh) || plug.isArray())
				{
					MFnTransform fnTransform(fnMesh.parent(0));
					int registerTransformId = idCheckTransform(fnTransform);

					bool check = false;
					MIntArray triangleCount;
					MIntArray triangleList;

					newVertices.id = idCheckVertex(fnMesh);
					newVertices.nrOfVertices = fnMesh.numVertices();
					
					MFloatPointArray pts;
					std::vector<Vertex> points;
					MIntArray vertexCounts;
					MIntArray polygonVertexIDs;
					MVector vertexNormal;
					MIntArray normalList, normalCount;
					MFloatArray u, v;
					MIntArray uvCounts;
					MIntArray uvIDs;
					MFloatVectorArray normals;

					MIntArray triangleCountsOffsets;
					MIntArray triangleIndices;

					fnMesh.getPoints(pts, MSpace::kObject);
					fnMesh.getUVs(u, v, 0);
					fnMesh.getAssignedUVs(uvCounts, uvIDs); //indices for UV:s

					fnMesh.getTriangleOffsets(triangleCountsOffsets, triangleIndices);
					fnMesh.getVertices(vertexCounts, polygonVertexIDs); //get vertex polygon indices
					fnMesh.getNormals(normals, MSpace::kObject);

					points.resize(triangleIndices.length());

					newVertices.nrOfIndices = triangleIndices.length();

					fnMesh.getNormalIds(normalCount, normalList);

					if (triangleIndices.length() > 0)
					{
						check = true;
					}

					for (int i = 0; i < triangleIndices.length(); i++) { //for each triangle index (36)

						points.at(i).x = pts[polygonVertexIDs[triangleIndices[i]]].x;
						points.at(i).y = pts[polygonVertexIDs[triangleIndices[i]]].y;
						points.at(i).z = pts[polygonVertexIDs[triangleIndices[i]]].z;

						points.at(i).nx = normals[normalList[triangleIndices[i]]].x;
						points.at(i).ny = normals[normalList[triangleIndices[i]]].y;
						points.at(i).nz = normals[normalList[triangleIndices[i]]].z;

						points.at(i).u = u[uvIDs[triangleIndices[i]]];
						points.at(i).v = abs(1- v[uvIDs[triangleIndices[i]]]);

					}

					// Write
					if (check)
					{
						memcpy(buff, &newVertices, sizeof(OurVertices));
						memcpy(buff + sizeof(OurVertices), points.data(), sizeof(Vertex) * triangleIndices.length());
						producer.write(buff, sizeof(OurVertices) + sizeof(Vertex) * triangleIndices.length());
					}
				}
			}
		}
	}
}

void GetMaterial(MObject &iteratorNode)
{
	MPlugArray shadingGoupArray;
	MPlugArray dagSetMemberConnections;
	MFnDependencyNode materialNode(iteratorNode);

	MPlug outColor = materialNode.findPlug("outColor"); //to go further in the plugs
	MPlug color = materialNode.findPlug("color"); //to get the color values

	//find surfaceShader of the material
	outColor.connectedTo(shadingGoupArray, false, true); //true = connection to source (outColor) 

	for (int i = 0; i < shadingGoupArray.length(); i++) {
		if (shadingGoupArray[i].node().hasFn(MFn::kShadingEngine)) {

			MFnDependencyNode shadingNode(shadingGoupArray[i].node());

			if (strcmp(shadingNode.name().asChar(), "initialParticleSE") != 0) {

				//ShadingNode = initialShadingGroup if lambert1 and Blinn1SG if blinn
				MPlug dagSetMember = shadingNode.findPlug("dagSetMembers");

				for (int child = 0; child < dagSetMember.numElements(); child++) {
					dagSetMember[child].connectedTo(dagSetMemberConnections, true, false); //true = connection to destination

					for (int d = 0; d < dagSetMemberConnections.length(); d++) {
						MFnDependencyNode dagSetMemberNode(dagSetMemberConnections[d].node());
						if (strcmp(dagSetMemberNode.name().asChar(), "shaderBallGeom1") != 0) {

							MFnMesh fnMesh(dagSetMemberNode.object());

							MGlobal::displayInfo("Mesh Name: " + fnMesh.name());

							MObjectArray shaders;
							MIntArray indices;
							fnMesh.getConnectedShaders(0, shaders, indices);
							for (uint i = 0; i < shaders.length(); i++)
							{
								MPlugArray connections;
								MFnDependencyNode shaderGroup(shaders[i]);
								MPlug shaderPlug = shaderGroup.findPlug("surfaceShader");
								shaderPlug.connectedTo(connections, true, false);
								for (uint u = 0; u < connections.length(); u++)
								{
									if (connections[u].node().hasFn(MFn::kLambert))
									{
										MPlugArray plugs;
										MFnLambertShader lambertShader(connections[u].node());
										lambertShader.findPlug("color").connectedTo(plugs, true, false);
										MColor mAmbientColor = lambertShader.ambientColor();
										MColor mColor = lambertShader.color();
										if (plugs.length() == 0)
										{
											OurMaterial material;
											material.id = idCheckVertex(fnMesh);
											material.color[0] = mColor.r;
											material.color[1] = mColor.g;
											material.color[2] = mColor.b;
											material.filePath[0] = '.';

											producer.write(&material, sizeof(OurMaterial));
										}

										for (uint y = 0; y < plugs.length(); y++)
										{
											MItDependencyGraph itDG(plugs[y]);
											MObject textureNode = itDG.thisNode();
											MPlug filenamePlug = MFnDependencyNode(textureNode).findPlug("fileTextureName");
											MString textureName;
											filenamePlug.getValue(textureName);

											if (checkTextureName(textureName, idCheckVertex(fnMesh)))
											{
												OurMaterial material;
												material.id = idCheckVertex(fnMesh);
												material.color[0] = 1;
												material.color[1] = 1;
												material.color[2] = 1;

												string tempString = textureName.asChar();
												strcpy(material.filePath, tempString.c_str());

												producer.write(&material, sizeof(OurMaterial));
											}
											else
											{
												OurMaterial material;
												material.id = idCheckVertex(fnMesh);
												material.color[0] = 1;
												material.color[1] = 1;
												material.color[2] = 1;
												material.filePath[0] = '.';

												producer.write(&material, sizeof(OurMaterial));
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
}

void UpdateMaterial(MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void* clientData)
{
	if (msg & MNodeMessage::AttributeMessage::kAttributeSet) {

		GetMaterial(plug.node());

	}
}

void cameraUpdated(const MString& str, void* clientData)
{
	OurCamera cameraInfo;
	M3dView viewPort;
	status = M3dView::getM3dViewFromModelPanel((char*)clientData, viewPort);
	MDagPath pathToUICamera;
	viewPort.getCamera(pathToUICamera);
	MFnCamera camera(pathToUICamera, &status);
	if (status == MS::kSuccess)
	{
		MPoint eyePoint = camera.eyePoint(MSpace::kWorld, &status);
		OUT(!MFAIL(status), eyePoint);
		float tempEye[4];
		eyePoint.get(tempEye);
		cameraInfo.eye[0] = tempEye[0];
		cameraInfo.eye[1] = tempEye[1];
		cameraInfo.eye[2] = tempEye[2];

		MPoint targetPoint = camera.centerOfInterestPoint(MSpace::kWorld, &status);
		OUT(!MFAIL(status), targetPoint);
		float tempTarget[4];
		targetPoint.get(tempTarget);
		cameraInfo.target[0] = tempTarget[0];
		cameraInfo.target[1] = tempTarget[1];
		cameraInfo.target[2] = tempTarget[2];

		MVector viewDirVector = camera.viewDirection(MSpace::kWorld, &status);
		OUT(!MFAIL(status), viewDirVector);
		float viewDir[3];
		cameraInfo.viewDir[0] = viewDirVector.x;
		cameraInfo.viewDir[1] = viewDirVector.y;
		cameraInfo.viewDir[2] = viewDirVector.z;

		MVector tempUp = camera.upDirection();
		cameraInfo.up[0] = tempUp.x;
		cameraInfo.up[1] = tempUp.y;
		cameraInfo.up[2] = tempUp.z;

		cameraInfo.orthoDist = camera.orthoWidth();
		cameraInfo.aspectRatio = camera.aspectRatio();

		if (camera.isOrtho())
		{
			cameraInfo.type = ORTHOGRAPHIC;
		}
		else
		{
			cameraInfo.type = PERSPECTIVE;
		}

		memcpy(buff, &cameraInfo, sizeof(OurCamera));
		producer.write(buff, sizeof(OurCamera));
	}
}

void nodeAdded(MObject &node, void * clientData) 
{
	callbackIdArray.append(MNodeMessage::addAttributeChangedCallback(node, nodeAttributeChanged, NULL, &status));

	MItDependencyNodes itDepNode(MFn::kLambert);

	while (itDepNode.isDone() == false)
	{
		MObject mNode = itDepNode.item();

		GetMaterial(mNode);

		MCallbackId MplugId = MNodeMessage::addAttributeChangedCallback(mNode, UpdateMaterial);

		itDepNode.next();
	}
}

static const char* panel1 = "modelPanel1";
static const char* panel2 = "modelPanel2";
static const char* panel3 = "modelPanel3";
static const char* panel4 = "modelPanel4";

EXPORT MStatus initializePlugin(MObject obj) {
	MStatus res = MS::kSuccess;
	MFnPlugin myPlugin(obj, "level editor", "1.0", "Any", &res);
	if (MFAIL(res)) {
		CHECK_MSTATUS(res);
		return res;
	}

	MGlobal::displayInfo("Plugin loaded ===========================");
	
	callbackIdArray.append(MDGMessage::addNodeAddedCallback(nodeAdded, "dependNode", NULL, &status));
	callbackIdArray.append(MUiMessage::add3dViewPreRenderMsgCallback(MString(panel1), cameraUpdated, (void*)panel1));
	callbackIdArray.append(MUiMessage::add3dViewPreRenderMsgCallback(MString(panel2), cameraUpdated, (void*)panel2));
	callbackIdArray.append(MUiMessage::add3dViewPreRenderMsgCallback(MString(panel3), cameraUpdated, (void*)panel3));
	callbackIdArray.append(MUiMessage::add3dViewPreRenderMsgCallback(MString(panel4), cameraUpdated, (void*)panel4));

	return res;
}
	

EXPORT MStatus uninitializePlugin(MObject obj) {
	MFnPlugin plugin(obj);
	MGlobal::displayInfo("Plugin unloaded =========================");
	MMessage::removeCallbacks(callbackIdArray);
	free(buff);
	return MS::kSuccess;
}