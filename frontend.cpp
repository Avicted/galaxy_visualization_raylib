// Defines -----------------------------------------------------------------------
#define RLIGHTS_IMPLEMENTATION

// Includes ----------------------------------------------------------------------
#include "includes/includes.h"
#include "includes/raylib_includes.h"

// Types -------------------------------------------------------------------------
struct ArcminData
{
    f32 right_ascension = 0.0f;
    f32 declination = 0.0f;
};

enum Draw_Data
{
    DRAW_DATA_A,
    DRAW_DATA_B,
    DRAW_ALL_DATA
};

// Variables ---------------------------------------------------------------------
i32 ScreenWidth = 640 * 2;
i32 ScreenHeight = 360 * 2;

bool Debug = false;
bool DataAIsLoaded = false;
bool IsPaused = false;
bool FreeLook = false;

i64 CPUMemory = 0L;

const char *DataAFilename = "./input_data/data_100k_arcmin.txt";
const char *DataBFilename = "./input_data/flat_100k_arcmin.txt";

Font MainFont = {0};

Camera3D MainCamera = {};
f32 Zoom = 1.3f;

const unsigned long int MAX_DATA_POINTS = 100000UL;

ArcminData *DataPointsA = nullptr;
ArcminData *DataPointsB = nullptr;

// Batch rendering in Raylib with a custom shader
Matrix *MatrixTransformsA = nullptr;
Matrix *MatrixTransformsB = nullptr;
Shader CustomShader = {0};

Draw_Data DataToDraw = DRAW_ALL_DATA;

// Define mesh to be instanced
Material matInstances;
Mesh SphereMesh;
// ----------------------------------------------------------------------------------

internal void
ParseInputArgs(i32 argc, char **argv)
{
    if (argc == 1)
    {
        printf("\tNo input args OK!\n");
        printf("\tCurrent working directory: %s\n", GetWorkingDirectory());
        return;
    }

    for (i32 i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "GALAXY_DEBUG") == 0)
        {
            printf("\tRunning in DEBUG mode !!!\n");
            Debug = true;
        }
    }
}

internal void
HandleWindowResize(void)
{
    if (IsWindowResized() && !IsWindowFullscreen())
    {
        ScreenWidth = GetScreenWidth();
        ScreenHeight = GetScreenHeight();
    }

    // Check for alt + enter
    if (IsKeyPressed(KEY_ENTER) && (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)))
    {
        // See which display we are on right now
        i32 Display = GetCurrentMonitor();

        if (IsWindowFullscreen())
        {
            // If we are full screen, then go back to the windowed size
            SetWindowSize(ScreenWidth, ScreenHeight);
        }
        else
        {
            // If we are not full screen, set the window size to match the monitor we are on
            SetWindowSize(GetMonitorWidth(Display), GetMonitorHeight(Display));
        }

        // Toggle the state
        ToggleFullscreen();

        ScreenWidth = GetScreenWidth();
        ScreenHeight = GetScreenHeight();
    }
}

internal void
RotateCameraAroundOrigo(f32 DeltaTime)
{
    Camera3D *Cam = &MainCamera;
    f32 Radius = 50.0f;
    f32 Speed = 0.2f;
    f32 VerticalSpeed = 0.1f; // Adjust this to control vertical sensitivity

    local_persist f32 PreviousTimeSinceStart = 0.0f;
    local_persist bool initializedFreeLook = false;
    local_persist Vector3 InitialCameraPosition;

    if (FreeLook)
    {
        if (!initializedFreeLook)
        {
            // Initialize the initial camera position based on the current camera position
            InitialCameraPosition = Cam->position;
            PreviousTimeSinceStart = atan2f(Cam->position.z, Cam->position.x); // Ensure continuity in rotation
            initializedFreeLook = true;
        }

        Vector2 mouseDelta = GetMouseDelta();
        PreviousTimeSinceStart += DeltaTime * Speed * mouseDelta.x;

        // Update camera position based on mouse delta (x for horizontal, y for vertical)
        Cam->position.x = InitialCameraPosition.x * cosf(PreviousTimeSinceStart) * Zoom;
        Cam->position.y = InitialCameraPosition.y + mouseDelta.y * VerticalSpeed;
        Cam->position.z = InitialCameraPosition.z * sinf(PreviousTimeSinceStart) * Zoom;

        Cam->target = Vector3Zero();
    }
    else
    {
        initializedFreeLook = false; // Reset the flag when not in free look mode

        PreviousTimeSinceStart += DeltaTime * Speed;

        Cam->position.x = Radius * cosf(PreviousTimeSinceStart) * Zoom;
        Cam->position.y = 50.0f;
        Cam->position.z = Radius * sinf(PreviousTimeSinceStart) * Zoom;

        Cam->target = Vector3Zero();
    }
}

internal void
GameUpdate(f32 DeltaTime)
{
    HandleWindowResize();

    if (IsKeyPressed(KEY_ESCAPE))
    {
        CloseWindow();
    }

    if (IsKeyPressed(KEY_F11))
    {
        ToggleFullscreen();
    }

    if (IsKeyPressed(KEY_ONE))
    {
        DataToDraw = DRAW_DATA_A;
    }

    if (IsKeyPressed(KEY_TWO))
    {
        DataToDraw = DRAW_DATA_B;
    }

    if (IsKeyPressed(KEY_THREE))
    {
        DataToDraw = DRAW_ALL_DATA;
    }

    if (IsKeyPressed(KEY_SPACE))
    {
        IsPaused = !IsPaused;
        FreeLook = !FreeLook;
        printf("\tIsPaused: %s\n", IsPaused ? "true" : "false");
    }

    RotateCameraAroundOrigo(DeltaTime);

    f32 Scroll = GetMouseWheelMove();
    if (Scroll != 0.0f)
    {
        const float ZoomChange = -2.5f;
        f32 Speed = ZoomChange;
        Zoom = Clamp(Zoom + Scroll * Speed * DeltaTime, 0.0f, 10.0f);
    }
}

internal void
GameRender(f32 DeltaTime)
{
    BeginDrawing();
    ClearBackground(BLACK);

    if (!DataAIsLoaded)
    {
        return;
    }

    // Draw the data around a sphere in 3D
    BeginMode3D(MainCamera);

    // Draw a earth sphere
    DrawSphere({0.0f, 0.0f, 0.0f}, 1.0f, BLUE);

    // Draw instanced meshes
    if (DataToDraw == DRAW_DATA_A || DataToDraw == DRAW_ALL_DATA)
    {
        matInstances.maps[MATERIAL_MAP_DIFFUSE].color = BLUE;
        DrawMeshInstanced(SphereMesh, matInstances, MatrixTransformsA, MAX_DATA_POINTS);
    }

    if (DataToDraw == DRAW_DATA_B || DataToDraw == DRAW_ALL_DATA)
    {
        matInstances.maps[MATERIAL_MAP_DIFFUSE].color = RED;
        DrawMeshInstanced(SphereMesh, matInstances, MatrixTransformsB, MAX_DATA_POINTS);
    }

    EndMode3D();

    // UI ------------------------------------------------------

    // Draw the FPS with our font
    DrawTextEx(MainFont, TextFormat("FPS: %i", GetFPS()), {10, 10}, 20, 2, WHITE);

    // Draw the camera position
    DrawTextEx(MainFont, TextFormat("Camera Position: (%.2f, %.2f, %.2f)", MainCamera.position.x, MainCamera.position.y, MainCamera.position.z), {10, 30}, 20, 2, WHITE);

    // Scroll to zoom
    DrawTextEx(MainFont, TextFormat("Scroll to zoom: %.2f", Zoom), {10, 50}, 20, 2, WHITE);

    // Press F11 to toggle fullscreen
    DrawTextEx(MainFont, TextFormat("Press F11 to toggle fullscreen"), {10, 70}, 20, 2, WHITE);

    // Press 1, 2 or 3 to toggle which data to draw
    DrawTextEx(MainFont, TextFormat("Press 1, 2 or 3 to toggle which data to draw"), {10, 90}, 20, 2, WHITE);

    // Red are uniformly distributed, blue are real data
    DrawTextEx(MainFont, TextFormat("Red are uniformly distributed"), {10, 110}, 20, 2, RED);
    DrawTextEx(MainFont, TextFormat("Blue are real data"), {10, 130}, 20, 2, BLUE);

    // Press space to pause in the center bottom
    if (IsPaused)
    {
        DrawTextEx(MainFont, TextFormat("Paused"), {ScreenWidth / 2.0f - 64.0f, ScreenHeight - 30.0f}, 20, 2, RED);
    }
    else
    {
        // Highlight the paused text
        DrawTextEx(MainFont, TextFormat("Auto Camera Active"), {ScreenWidth / 2.0f - 128.0f, ScreenHeight - 30.0f}, 20, 2, GREEN);
    }

    if (FreeLook)
    {
        const char *FreeLookText = "Free Look";
        const int FreeLookTextLength = strlen(FreeLookText);
        DrawTextEx(MainFont, FreeLookText, {ScreenWidth - 128.0f - 64.0f, 30}, 20, 2, GREEN);
    }
    else
    {
        const char *FreeLookText = "Auto Look";
        const int FreeLookTextLength = strlen(FreeLookText);
        DrawTextEx(MainFont, FreeLookText, {ScreenWidth - 64.0f - 128.0f - 32.0f, 30}, 20, 2, RED);
    }

    EndDrawing();
}

internal void
PrintMemoryUsage(void)
{
    printf("\n\tMemory used in GigaBytes: %f\n", (f32)CPUMemory / (f32)Gigabytes(1));
    printf("\tMemory used in MegaBytes: %f\n", (f32)CPUMemory / (f32)Megabytes(1));
}

internal void
CleanupOurStuff(void)
{
    CloseWindow(); // Close window and OpenGL context
    printf("\n\tClosed window and OpenGL context\n");

    free(DataPointsA);
    CPUMemory -= MAX_DATA_POINTS * sizeof(ArcminData);
    printf("\n\tFreeing DataPointsA %lu\n", MAX_DATA_POINTS * sizeof(ArcminData));
    PrintMemoryUsage();

    free(DataPointsB);
    CPUMemory -= MAX_DATA_POINTS * sizeof(ArcminData);
    printf("\n\tFreeing DataPointsB: %lu\n", MAX_DATA_POINTS * sizeof(ArcminData));
    PrintMemoryUsage();

    free(MatrixTransformsA);
    CPUMemory -= MAX_DATA_POINTS * sizeof(Matrix);
    printf("\n\tFreeing MatrixTransformsA: %lu\n", MAX_DATA_POINTS * sizeof(Matrix));
    PrintMemoryUsage();

    free(MatrixTransformsB);
    CPUMemory -= MAX_DATA_POINTS * sizeof(Matrix);
    printf("\n\tFreeing MatrixTransformsB: %lu\n", MAX_DATA_POINTS * sizeof(Matrix));
    PrintMemoryUsage();

    // @Note(Victor): There should be no allocated memory left
    Assert(CPUMemory == 0);
}

internal void
SigIntHandler(i32 Signal)
{
    printf("\tCaught SIGINT, exiting peacefully!\n");

    CleanupOurStuff();

    exit(0);
}

local_persist bool
ReadInputDataFromFile(const char *FileName, ArcminData *DataPointsLocation)
{
    FILE *f = fopen(FileName, "r");
    if (f == NULL)
    {
        printf("Error opening file!\n");
        return (false);
    }

    // read the header
    char Line[1024]; // Adjust size as needed

    if (fgets(Line, sizeof(Line), f) == NULL)
    {
        printf("Error reading header!\n");
        return (false);
    }

    // Read the data into the DataPointsLocation the data is in arcmin declination and right ascension \t separated
    i32 i = 0;
    while (fgets(Line, sizeof(Line), f) != NULL)
    {
        // printf("Retrieved line of length %zu:\n", Read);
        // printf("%s", Line);

        // @Note(Victor): We expect the input data to be separated by tabs !!!
        // Parse the line
        char *Token = strtok(Line, "\t");
        i32 j = 0;
        while (Token != NULL)
        {
            // printf("Token: %s\n", Token);

            if (j == 0)
            {
                DataPointsLocation[i].right_ascension = atof(Token);
            }
            else if (j == 1)
            {
                DataPointsLocation[i].declination = atof(Token);
            }
            else
            {
                printf("Error parsing line!\n");
                return (false);
            }

            Token = strtok(NULL, "\t");
            j++;
        }

        i++;
    }

    fclose(f);

    return (true);
}

i32 main(i32 argc, char **argv)
{
    signal(SIGINT, SigIntHandler);

    ParseInputArgs(argc, argv);

    // Allocate the memory for the data with calloc
    DataPointsA = (ArcminData *)calloc(MAX_DATA_POINTS, sizeof(ArcminData));
    CPUMemory += MAX_DATA_POINTS * sizeof(ArcminData);

    DataPointsB = (ArcminData *)calloc(MAX_DATA_POINTS, sizeof(ArcminData));
    CPUMemory += MAX_DATA_POINTS * sizeof(ArcminData);

    if (ReadInputDataFromFile(DataAFilename, DataPointsA))
    {
        printf("\tReadInputDataFromFile: %s succeeded!\n", DataAFilename);
    }
    else
    {
        printf("\tReadInputDataFromFile: %s failed!\n", DataAFilename);
        CleanupOurStuff();
        return (1);
    }

    if (ReadInputDataFromFile(DataBFilename, DataPointsB))
    {
        printf("\tReadInputDataFromFile: %s succeeded!\n", DataBFilename);
    }
    else
    {
        printf("\tReadInputDataFromFile: %s failed!\n", DataBFilename);
        CleanupOurStuff();
        return (1);
    }

    printf("\tHello from raylib_galaxy_application!\n\n");

    unsigned long int Count = 0;
    for (unsigned long int i = 0; i < MAX_DATA_POINTS; ++i)
    {
        if (DataPointsA[i].right_ascension != 0.0f)
        {
            Count++;
        }
    }

    Assert(Count == MAX_DATA_POINTS);
    Assert(DataPointsB != NULL);

    Count = 0;
    for (unsigned long int i = 0; i < MAX_DATA_POINTS; ++i)
    {
        if (DataPointsB[i].right_ascension != 0.0f)
        {
            Count++;
        }
    }

    Assert(Count == MAX_DATA_POINTS);

    DataAIsLoaded = true;

    // Set the camera to rotate around the center of the data
    MainCamera.position = {0.0f, 50.0f, 100.0f};
    MainCamera.target = {0.0f, 0.0f, 0.0f};
    MainCamera.up = {0.0f, 1.0f, 0.0f};
    MainCamera.fovy = 75.0f; // Adjust if necessary
    MainCamera.projection = CAMERA_PERSPECTIVE;

    // Define transforms to be uploaded to GPU for instances
    {
        MatrixTransformsA = (Matrix *)calloc(MAX_DATA_POINTS, sizeof(Matrix));
        CPUMemory += MAX_DATA_POINTS * sizeof(Matrix);

        MatrixTransformsB = (Matrix *)calloc(MAX_DATA_POINTS, sizeof(Matrix));
        CPUMemory += MAX_DATA_POINTS * sizeof(Matrix);

        Matrix rotation = MatrixRotateXYZ({0.0f, 0.0f, 0.0f});

        for (unsigned long int i = 0; i < MAX_DATA_POINTS; ++i)
        {
            // DataPointsA real galaxies
            {
                // Transform the arc minutes into radians that the trigonometric functions take as input. (sinf, cosf, tanf)
                f32 RightAscensionRad = (DataPointsA[i].right_ascension / 60.0f) * (PI / 180.0f);
                f32 DeclinationRad = (DataPointsA[i].declination / 60.0f) * (PI / 180.0f);

                // Calculate the position on the sphere using spherical coordinates
                f32 Radius = 50.0f;
                f32 X = Radius * cosf(RightAscensionRad) * cosf(DeclinationRad);
                f32 Y = Radius * sinf(DeclinationRad);
                f32 Z = Radius * sinf(RightAscensionRad) * cosf(DeclinationRad);

                // Create a model matrix for each data point to position it
                MatrixTransformsA[i] = MatrixIdentity();
                MatrixTransformsA[i] = MatrixMultiply(MatrixTransformsA[i], MatrixScale(0.1f, 0.1f, 0.1f));
                MatrixTransformsA[i] = MatrixMultiply(MatrixTransformsA[i], MatrixTranslate(X, Y, Z));
            }

            // DataPointsB uniformly distributed (galaxies)
            {
                f32 RightAscensionRad = (DataPointsB[i].right_ascension / 60.0f) * (PI / 180.0f);
                f32 DeclinationRad = (DataPointsB[i].declination / 60.0f) * (PI / 180.0f);

                // Calculate the position on the sphere using spherical coordinates
                f32 Radius = 50.0f;
                f32 X = Radius * cosf(RightAscensionRad) * cosf(DeclinationRad);
                f32 Y = Radius * sinf(DeclinationRad);
                f32 Z = Radius * sinf(RightAscensionRad) * cosf(DeclinationRad);

                // Create a model matrix for each data point to position it
                MatrixTransformsB[i] = MatrixIdentity();
                MatrixTransformsB[i] = MatrixMultiply(MatrixTransformsB[i], MatrixScale(0.1f, 0.1f, 0.1f));
                MatrixTransformsB[i] = MatrixMultiply(MatrixTransformsB[i], MatrixTranslate(X, Y, Z));
            }
        }
    }

    // Raylib
    {
        SetTraceLogLevel(LOG_WARNING);
        SetConfigFlags(FLAG_WINDOW_RESIZABLE);
        InitWindow(ScreenWidth, ScreenHeight, "galaxy_visuazation_raylib");

#if defined(PLATFORM_WEB)
        emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
        SetTargetFPS(60);
    }

    MainFont = LoadFontEx("./fonts/SuperMarioBros2.ttf", 32, 0, 250);
    CustomShader = LoadShader("shaders/lighting_instancing.vs", "shaders/lighting.fs");
    SphereMesh = GenMeshSphere(0.2f, 16, 16);

    // Get shader locations
    CustomShader.locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(CustomShader, "mvp");
    CustomShader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(CustomShader, "viewPos");
    CustomShader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocationAttrib(CustomShader, "instanceTransform");

    // Lighting
    {
        // Set shader value: ambient light level
        i32 AmbientLoc = GetShaderLocation(CustomShader, "ambient");
        f32 AmbientValue[4] = {0.2f, 0.2f, 0.2f, 1.0f};
        SetShaderValue(CustomShader, AmbientLoc, &AmbientValue, SHADER_UNIFORM_VEC4);

        CreateLight(LIGHT_DIRECTIONAL, {500.0f, 500.0f, 0.0f}, Vector3Zero(), WHITE, CustomShader);
    }

    // Material
    {
        // NOTE: We are assigning the intancing shader to material.shader
        // to be used on mesh drawing with DrawMeshInstanced()
        matInstances = LoadMaterialDefault();
        matInstances.shader = CustomShader;
        matInstances.maps[MATERIAL_MAP_DIFFUSE].color = BLUE;
    }

    printf("\n\tMemory usage before we start the game loop\n");
    PrintMemoryUsage();

    // Main loop
    while (!WindowShouldClose()) // Detect window close button or ESC key
    {
        f32 DeltaTime = GetFrameTime();
        GameUpdate(DeltaTime);
        GameRender(DeltaTime);
    }
#endif
        CleanupOurStuff();

        return (0);
    }
