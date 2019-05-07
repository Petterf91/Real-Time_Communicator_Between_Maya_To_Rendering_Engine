#include <vector>
#include <stdio.h>
#include <iostream>
#include "include/raylib.h"
#include "include/raymath.h"
#include "include/rlgl.h"
#include <ComLib.h>

using namespace std;

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

struct OurTransform {
	int messagetype = TRANSFORM;
	int id;
	float translation[3];
	float rotation[3];
	float scale[3];
};

struct modelPos { 
	Model model; 
	Matrix modelMatrix; 
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

size_t memorySize;
ComLib consumer("sharedMem", (size_t)10, ComLib::TYPE::CONSUMER);

int bufferCounter = 0;

int main()
{
	SetTraceLog(LOG_WARNING);

    // Initialization
    //--------------------------------------------------------------------------------------
    int screenWidth = 1200;
    int screenHeight = 800;
    
    SetConfigFlags(FLAG_MSAA_4X_HINT);      // Enable Multi Sampling Anti Aliasing 4x (if available)
	SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(screenWidth, screenHeight, "Game Engine");

    // Define the camera to look into our 3d world
	Camera camera = { 0 };
    camera.position = { 4.0f, 4.0f, 4.0f };
    camera.target = { 0.0f, 1.0f, -1.0f };
    camera.up = { 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.type = CAMERA_PERSPECTIVE;

	
	// real models rendered each frame
	std::vector<modelPos> flatScene;

	Material material1 = LoadMaterialDefault();
    Texture2D texture1 = LoadTexture("resources/models/watermill_diffuse.png");   

    Shader shader1 = LoadShader("resources/shaders/glsl330/phong.vs", 
                               "resources/shaders/glsl330/phong.fs");   // Load model shader
	material1.shader = shader1;

	Mesh mesh1 = LoadMesh("resources/models/watermill.obj");
    Model model1 = LoadModelFromMesh(mesh1);
    model1.material = material1;                     // Set shader effect to 3d model


	//Try to import Cube
	char* buffer;
	buffer = (char*)malloc(1024 * 1024 * 10);

    Vector3 position = { 0.0f, 0.0f, 0.0f };    // Set model position
    SetCameraMode(camera, CAMERA_CUSTOM);         // Set an orbital camera mode
    SetTargetFPS(500);                           // Set our game to run at 60 frames-per-second

	int modelLoc = GetShaderLocation(shader1, "model");
	int viewLoc = GetShaderLocation(shader1, "view");
	int projLoc = GetShaderLocation(shader1, "projection");

	int lightLoc = GetShaderLocation(shader1, "lightPos");
	Vector3 lightPos = { 0, 4, 0 };
	Matrix viewMat;
	Matrix projMat;
    
	Matrix projection = MatrixPerspective(0.78, GetScreenWidth() / (float)GetScreenHeight(),0.1,1000);
	SetMatrixProjection(projection);

    // Main game loop
    while (!WindowShouldClose())                // Detect window close button or ESC key
    {
		while (bufferCounter < 20)
		{
			if (consumer.read(buffer, memorySize))
			{
				int messageType;
				memcpy(&messageType, buffer, sizeof(int));

				if (messageType == TRANSFORM)
				{
					OurTransform transformStruct = {};
					transformStruct.id = *((int*)(buffer + sizeof(int)));

					memcpy(&transformStruct.translation[0], buffer + sizeof(int) + sizeof(int), sizeof(float) * 3);
					memcpy(&transformStruct.rotation[0], buffer + sizeof(int) + sizeof(int) + sizeof(float) * 3, sizeof(float) * 3);
					memcpy(&transformStruct.scale[0], buffer + sizeof(int) + sizeof(int) + sizeof(float) * 6, sizeof(float) * 3);

					Matrix rotationZ = MatrixRotate({ 0,0,1 }, transformStruct.rotation[2]);
					Matrix rotationY = MatrixRotate({ 0,1,0 }, transformStruct.rotation[1]);
					Matrix rotationX = MatrixRotate({ 1,0,0 }, transformStruct.rotation[0]);
					Matrix rotationMatrix = MatrixMultiply(MatrixMultiply(rotationX, rotationY), rotationZ);

					Matrix tempModelmatrix = MatrixMultiply(MatrixMultiply(
						MatrixScale(transformStruct.scale[0], transformStruct.scale[1], transformStruct.scale[2]),
						rotationMatrix),
						MatrixTranslate(transformStruct.translation[0], transformStruct.translation[1], transformStruct.translation[2]));

					flatScene[transformStruct.id].modelMatrix = tempModelmatrix;

				}
				else if (messageType == VERTICES)
				{
					OurVertices vertexInfo = {};
					vector<Vertex> vertices;
					Vertex tempVtx;

					vertexInfo.id = *((int*)(buffer + sizeof(int)));
					vertexInfo.nrOfVertices = *((int*)(buffer + sizeof(int) + sizeof(int)));
					vertexInfo.nrOfIndices = *((int*)(buffer + sizeof(int) + sizeof(int) * 2));

					for (int i = 0; i < vertexInfo.nrOfIndices; i++)
					{
						memcpy(&tempVtx, buffer + sizeof(OurVertices) + sizeof(Vertex) * i, sizeof(Vertex));
						vertices.push_back(tempVtx);
					}

					Mesh mesh3 = {};
					mesh3.vertexCount = vertexInfo.nrOfIndices;
					mesh3.triangleCount = vertexInfo.nrOfIndices / 3;
					mesh3.vertices = new float[vertexInfo.nrOfIndices * 3];
					mesh3.normals = new float[vertexInfo.nrOfIndices * 3];
					mesh3.texcoords = new float[vertexInfo.nrOfIndices * 2];
					Vertex temp;
					int u = 0;
					int w = 0;

					for (int i = 0; i < vertexInfo.nrOfIndices; i++)
					{
						memcpy(&temp, &vertices[i], sizeof(float) * 8);
						mesh3.vertices[u] = temp.x;
						mesh3.vertices[u + 1] = temp.y;
						mesh3.vertices[u + 2] = temp.z;
						mesh3.normals[u] = temp.nx;
						mesh3.normals[u + 1] = temp.ny;
						mesh3.normals[u + 2] = temp.nz;
						mesh3.texcoords[w] = temp.u;
						mesh3.texcoords[w + 1] = temp.v;
						w += 2;
						u += 3;
					}

					rlLoadMesh(&mesh3, false);

					if (flatScene.size() <= vertexInfo.id)
					{
						Model customModel = LoadModelFromMesh(mesh3);
						customModel.material = material1;
						flatScene.push_back({ customModel, MatrixTranslate(0,0,0) });
					}
					else
					{
						flatScene[vertexInfo.id].model.mesh = mesh3;
					}
				}
				else if (messageType == MATERIAL)
				{
					OurMaterial materialStruct;
					memcpy(&materialStruct, buffer, sizeof(OurMaterial));

					material1.maps[MAP_DIFFUSE].color.r = (char)(materialStruct.color[0] * 255);
					material1.maps[MAP_DIFFUSE].color.g = (char)(materialStruct.color[1] * 255);
					material1.maps[MAP_DIFFUSE].color.b = (char)(materialStruct.color[2] * 255);
					material1.maps[MAP_DIFFUSE].color.a = (char)255;

					if (materialStruct.filePath[0] != '.')
					{
						texture1 = LoadTexture(materialStruct.filePath);
						material1.maps[MAP_DIFFUSE].texture = texture1;
					}

					material1.shader = shader1;
					flatScene[materialStruct.id].model.material = material1;
				}
				else if (messageType == LIGHTPOS)
				{
					memcpy(&lightPos, buffer + sizeof(int), sizeof(float) * 3);
				}
				else if (messageType == CAMERA)
				{
					OurCamera cameraInfo = {};

					memcpy(&camera.type, buffer + sizeof(int), sizeof(int));
					memcpy(&camera.position, buffer + sizeof(int) * 2, sizeof(float) * 3);
					memcpy(&camera.target, buffer + sizeof(int) * 2 + sizeof(float) * 3, sizeof(float) * 3);
					memcpy(&camera.up, buffer + sizeof(int) * 2 + sizeof(float) * 6, sizeof(float) * 3);
				}
			}
			if (consumer.read(buffer, memorySize))
			{
				int messageType;
				memcpy(&messageType, buffer, sizeof(int));

				if (messageType == TRANSFORM)
				{
					OurTransform transformStruct = {};
					transformStruct.id = *((int*)(buffer + sizeof(int)));

					memcpy(&transformStruct.translation[0], buffer + sizeof(int) + sizeof(int), sizeof(float) * 3);
					memcpy(&transformStruct.rotation[0], buffer + sizeof(int) + sizeof(int) + sizeof(float) * 3, sizeof(float) * 3);
					memcpy(&transformStruct.scale[0], buffer + sizeof(int) + sizeof(int) + sizeof(float) * 6, sizeof(float) * 3);

					Matrix rotationZ = MatrixRotate({ 0,0,1 }, transformStruct.rotation[2]);
					Matrix rotationY = MatrixRotate({ 0,1,0 }, transformStruct.rotation[1]);
					Matrix rotationX = MatrixRotate({ 1,0,0 }, transformStruct.rotation[0]);
					Matrix rotationMatrix = MatrixMultiply(MatrixMultiply(rotationX, rotationY), rotationZ);

					Matrix tempModelmatrix = MatrixMultiply(MatrixMultiply(
						MatrixScale(transformStruct.scale[0], transformStruct.scale[1], transformStruct.scale[2]),
						rotationMatrix),
						MatrixTranslate(transformStruct.translation[0], transformStruct.translation[1], transformStruct.translation[2]));

					flatScene[transformStruct.id].modelMatrix = tempModelmatrix;

				}
				else if (messageType == VERTICES)
				{
					OurVertices vertexInfo = {};
					vector<Vertex> vertices;
					Vertex tempVtx;

					vertexInfo.id = *((int*)(buffer + sizeof(int)));
					vertexInfo.nrOfVertices = *((int*)(buffer + sizeof(int) + sizeof(int)));
					vertexInfo.nrOfIndices = *((int*)(buffer + sizeof(int) + sizeof(int) * 2));

					for (int i = 0; i < vertexInfo.nrOfIndices; i++)
					{
						memcpy(&tempVtx, buffer + sizeof(OurVertices) + sizeof(Vertex) * i, sizeof(Vertex));
						vertices.push_back(tempVtx);
					}

					Mesh mesh3 = {};
					mesh3.vertexCount = vertexInfo.nrOfIndices;
					mesh3.triangleCount = vertexInfo.nrOfIndices / 3;
					mesh3.vertices = new float[vertexInfo.nrOfIndices * 3];
					mesh3.normals = new float[vertexInfo.nrOfIndices * 3];
					mesh3.texcoords = new float[vertexInfo.nrOfIndices * 2];
					Vertex temp;
					int u = 0;
					int w = 0;

					for (int i = 0; i < vertexInfo.nrOfIndices; i++)
					{
						memcpy(&temp, &vertices[i], sizeof(float) * 8);
						mesh3.vertices[u] = temp.x;
						mesh3.vertices[u + 1] = temp.y;
						mesh3.vertices[u + 2] = temp.z;
						mesh3.normals[u] = temp.nx;
						mesh3.normals[u + 1] = temp.ny;
						mesh3.normals[u + 2] = temp.nz;
						mesh3.texcoords[w] = temp.u;
						mesh3.texcoords[w + 1] = temp.v;
						w += 2;
						u += 3;
					}

					rlLoadMesh(&mesh3, false);

					if (flatScene.size() <= vertexInfo.id)
					{
						Model customModel = LoadModelFromMesh(mesh3);
						customModel.material = material1;
						flatScene.push_back({ customModel, MatrixTranslate(0,0,0) });
					}
					else
					{
						flatScene[vertexInfo.id].model.mesh = mesh3;
					}
				}
				else if (messageType == MATERIAL)
				{
					OurMaterial materialStruct;
					memcpy(&materialStruct, buffer, sizeof(OurMaterial));

					material1.maps[MAP_DIFFUSE].color.r = (char)(materialStruct.color[0] * 255);
					material1.maps[MAP_DIFFUSE].color.g = (char)(materialStruct.color[1] * 255);
					material1.maps[MAP_DIFFUSE].color.b = (char)(materialStruct.color[2] * 255);
					material1.maps[MAP_DIFFUSE].color.a = (char)255;

					if (materialStruct.filePath[0] != '.')
					{
						texture1 = LoadTexture(materialStruct.filePath);
						material1.maps[MAP_DIFFUSE].texture = texture1;
					}

					material1.shader = shader1;
					flatScene[materialStruct.id].model.material = material1;
				}
				else if (messageType == LIGHTPOS)
				{
					memcpy(&lightPos, buffer + sizeof(int), sizeof(float) * 3);
				}
				else if (messageType == CAMERA)
				{
					OurCamera cameraInfo = {};

					memcpy(&camera.type, buffer + sizeof(int), sizeof(int));
					memcpy(&camera.position, buffer + sizeof(int) * 2, sizeof(float) * 3);
					memcpy(&camera.target, buffer + sizeof(int) * 2 + sizeof(float) * 3, sizeof(float) * 3);
					memcpy(&camera.up, buffer + sizeof(int) * 2 + sizeof(float) * 6, sizeof(float) * 3);
				}

			}
			bufferCounter++;
		}

		bufferCounter = 0;

        BeginDrawing();

            BeginMode3D(camera);
            ClearBackground(GRAY);
			
			DrawSphere(lightPos, 0.1, GREEN);

			for (int i = 0; i < flatScene.size(); i++)
			{
				auto m = flatScene[i];
				BeginShaderMode(m.model.material.shader);

				auto l = GetShaderLocation(m.model.material.shader, "model");
				SetShaderValue(shader1, lightLoc, Vector3ToFloat(lightPos), 3);

				SetShaderValueMatrix(m.model.material.shader, modelLoc, m.modelMatrix);
				DrawModel(m.model,{}, 1.0, m.model.material.maps[MAP_DIFFUSE].color);

				EndShaderMode();
			}

            EndMode3D();

        EndDrawing();
    }

    // De-Initialization
    UnloadShader(shader1);       // Unload shader
    UnloadTexture(texture1);     // Unload texture
	UnloadModel(model1);

	// rlUnloadMesh(&mesh2);
	//CloseWindow();              // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}