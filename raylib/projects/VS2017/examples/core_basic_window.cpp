#include <vector>
#include <stdio.h>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>

#include <Windows.h>

struct modelPos { 
	Model model; 
	Matrix modelMatrix; 
};

int main()
{
	SetTraceLog(LOG_WARNING);

    // Initialization
    //--------------------------------------------------------------------------------------
    int screenWidth = 1200;
    int screenHeight = 800;
    
    SetConfigFlags(FLAG_MSAA_4X_HINT);      // Enable Multi Sampling Anti Aliasing 4x (if available)
	SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(screenWidth, screenHeight, "demo");

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
	material1.maps[MAP_DIFFUSE].texture = texture1;

    Shader shader1 = LoadShader("resources/shaders/glsl330/phong.vs", 
                               "resources/shaders/glsl330/phong.fs");   // Load model shader
	material1.shader = shader1;

	Mesh mesh1 = LoadMesh("resources/models/watermill.obj");
    Model model1 = LoadModelFromMesh(mesh1);                   
    model1.material = material1;                     // Set shader effect to 3d model


	// triangle by hand
	float vtx[3*3]{ 
		1, 0, 1,
		1, 0, 0,
		0, 0, 1 
	};
	float norm[9] { 0, 1, 0, 0, 1, 0, 0, 1, 0};
	unsigned short indices[3] = { 0, 1, 2 }; // remove to use vertices only
	unsigned char colors[12] = { 255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255 };

    Vector3 position = { 0.0f, 0.0f, 0.0f };    // Set model position
    SetCameraMode(camera, CAMERA_FREE);         // Set an orbital camera mode
    SetTargetFPS(60);                           // Set our game to run at 60 frames-per-second

	int modelLoc = GetShaderLocation(shader1, "model");
	int viewLoc = GetShaderLocation(shader1, "view");
	int projLoc = GetShaderLocation(shader1, "projection");

	int lightLoc = GetShaderLocation(shader1, "lightPos");
	Vector3 lightPos = {4,1,4};

	Mesh mesh2 = {};
	mesh2.vertexCount = 3;
	mesh2.triangleCount = 1;
	mesh2.vertices = new float[9]; 
	mesh2.normals = new float[9];
	mesh2.indices = new unsigned short[3]; // remove to use vertices only
	mesh2.colors = new unsigned char[12];

	memcpy(mesh2.vertices, vtx, sizeof(float)*9);
	memcpy(mesh2.normals, norm, sizeof(float)*9);
	memcpy(mesh2.indices, indices, sizeof(unsigned short)*3); // remove to use vertices only
	memcpy(mesh2.colors, colors, sizeof(unsigned char)*12); // remove to use vertices only

	rlLoadMesh(&mesh2, false);

	Model tri = LoadModelFromMesh(mesh2);
	tri.material.shader = shader1;
	flatScene.push_back({tri, MatrixTranslate(2,0,2)});
    
    // Main game loop
    while (!WindowShouldClose())                // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        UpdateCamera(&camera);                  // Update camera
        //----------------------------------------------------------------------------------
        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

			SetShaderValueMatrix(shader1, viewLoc, GetCameraMatrix(camera));
			SetShaderValue(shader1, lightLoc, Vector3ToFloat(lightPos), 3);

            ClearBackground(RAYWHITE);

			if (IsKeyPressed(KEY_D))
			{
				Vector3 pos = { (rand() / (float)RAND_MAX) * 10.0f, 0.0f, (rand() / (float)RAND_MAX) * 10.0f };

				static int i = 0;

				// matrices are transposed later, so order here would be [Translate*Scale*vector]
				Matrix modelmatrix = MatrixMultiply( MatrixMultiply( 
					MatrixRotate({0,1,0}, i / (float)5), 
					MatrixScale(0.1,0.1,0.1)),
					MatrixTranslate(pos.x,pos.y,pos.z));

				Model copy = model1;
				flatScene.push_back({copy, modelmatrix});
				i++;
			}

            BeginMode3D(camera);

			for (int i = 0; i < flatScene.size(); i++)
			{
				auto m = flatScene[i];
				auto l = GetShaderLocation(m.model.material.shader, "model");
				SetShaderValueMatrix( m.model.material.shader, modelLoc, m.modelMatrix);
				DrawModel(m.model,{}, 1.0, WHITE);
			}
            DrawGrid(10, 1.0f);     // Draw a grid

			DrawSphere(lightPos, 0.1,RED);

            EndMode3D();

            DrawTextRL("(c) Watermill 3D model by Alberto Cano", screenWidth - 210, screenHeight - 20, 10, GRAY);
            DrawTextRL(FormatText("Camera position: (%.2f, %.2f, %.2f)", camera.position.x, camera.position.y, camera.position.z), 600, 20, 10, BLACK);
            DrawTextRL(FormatText("Camera target: (%.2f, %.2f, %.2f)", camera.target.x, camera.target.y, camera.target.z), 600, 40, 10, GRAY);
            DrawFPS(10, 10);
        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    UnloadShader(shader1);       // Unload shader
    UnloadTexture(texture1);     // Unload texture
	UnloadModel(model1);
	UnloadModel(tri);

    CloseWindowRL();              // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}