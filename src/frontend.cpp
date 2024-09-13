// Defines -----------------------------------------------------------------------
#define RLIGHTS_IMPLEMENTATION

// Includes ----------------------------------------------------------------------
#include "includes.h"
#include "raylib_includes.h"

// Types -------------------------------------------------------------------------
struct ArcminData
{
    f64 right_ascension = 0.0f;
    f64 declination = 0.0f;
    f64 redshift = 0.0f;
};

enum Draw_Data
{
    // Draw the data from the files from the ÅA course
    DRAW_DATA_A,
    DRAW_DATA_B,
    DRAW_ALL_DATA,

    // Draw the redshift data, not from the course
    DRAW_REDSHIFT_DATA,
};

// Variables ---------------------------------------------------------------------
i32 SCREEN_WIDTH = 640 * 2;
i32 SCREEN_HEIGHT = 360 * 2;

bool Debug = false;
bool DataAIsLoaded = false;
bool IsPaused = false;

u64 CPUMemory = 0L;

constexpr f64 PIdividedBy180 = (PI / 180.0);

const char *DataAFilename = "./input_data/data_100k_arcmin.txt";
const char *DataBFilename = "./input_data/flat_100k_arcmin.txt";
const char *RedshiftDataFilename = "./redshift_input_data/seyfert.dat";

Font MainFont = {0};

Camera3D MainCamera = {};
f64 Zoom = 1.0f * PI;

const unsigned long int MAX_DATA_POINTS = 100000UL;
unsigned long int MAX_REDSHIFT_DATA_POINTS = 100000UL; // @Note(Victor): This is set when we read the redshift data

// @Note(Victor): Data from the course, only celestial coordinates, no redshift (distance)
ArcminData *DataPointsA = nullptr;
ArcminData *DataPointsB = nullptr;

// @Note(Victor): Data from the redshift file with the appriximated distances to the galaxies
ArcminData *RedshiftData = nullptr;

// Batch rendering in Raylib with a custom shader
Matrix *MatrixTransformsA = nullptr;
Matrix *MatrixTransformsB = nullptr;
Matrix *MatrixTransformsRedshift = nullptr;

Shader CustomShader = {0};

Draw_Data DataToDraw = DRAW_ALL_DATA;

// Define mesh to be instanced
Material matInstances;
Mesh SphereMesh;

// 3D Models
Model EarthModel;

// Redshift data calculations
// ----------------------------------------------------------------------------------
// Function to convert RA from HHMMSS to degrees
internal f64
ConvertRaToDegrees(f64 raHHMMSS)
{
    int hours = (int)(raHHMMSS / 10000);
    int minutes = (int)((raHHMMSS - (hours * 10000)) / 100);
    f64 seconds = raHHMMSS - (hours * 10000) - (minutes * 100);

    return 15.0 * (hours + (minutes / 60.0) + (seconds / 3600.0)); // 1 hour = 15 degrees
}

// Function to convert DEC from DDMMSS to degrees
// DEC: Declination
// DDMMSS: Degrees, minutes, seconds
internal f64
ConvertDecToDegrees(f64 decDDMMSS)
{
    int degrees = (int)(decDDMMSS / 10000);
    int minutes = (int)((decDDMMSS - (degrees * 10000)) / 100);
    f64 seconds = decDDMMSS - (degrees * 10000) - (minutes * 100);

    f64 decDegrees = abs(degrees) + (minutes / 60.0) + (seconds / 3600.0);
    return (degrees < 0) ? -decDegrees : decDegrees;
}

// Assuming speed of light in km/s for converting redshift to distance (simplified calculation)
const f64 speedOfLight = 299792.458; // Speed of light in km/s
const f64 hubbleConstant = 70.0;     // Hubble constant in km/s/Mpc

internal f64
RedshiftToDistance(f64 redshift)
{
    // Distance in Megaparsecs (Mpc)
    return (speedOfLight * redshift) / hubbleConstant;
}

// Convert spherical coordinates (RA, Dec, distance) to Cartesian (X, Y, Z)
internal void
CalculatePosition(f64 ra, f64 dec, f64 redshift, f64 &X, f64 &Y, f64 &Z)
{
    f64 distance = RedshiftToDistance(redshift); // Convert redshift to distance (Mpc)

    // Convert degrees to radians
    f64 raRad = ra * PIdividedBy180;
    f64 decRad = dec * PIdividedBy180;

    // Calculate Cartesian coordinates
    X = distance * cos(decRad) * cos(raRad);
    Y = distance * cos(decRad) * sin(raRad);
    Z = distance * sin(decRad);
}
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
        SCREEN_WIDTH = GetScreenWidth();
        SCREEN_HEIGHT = GetScreenHeight();
    }

    // Check for alt + enter
    if (IsKeyPressed(KEY_ENTER) && (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)))
    {
        // See which display we are on right now
        i32 Display = GetCurrentMonitor();

        if (IsWindowFullscreen())
        {
            // If we are full screen, then go back to the windowed size
            SetWindowSize(SCREEN_WIDTH, SCREEN_HEIGHT);
        }
        else
        {
            // If we are not full screen, set the window size to match the monitor we are on
            SetWindowSize(GetMonitorWidth(Display), GetMonitorHeight(Display));
        }

        // Toggle the state
        ToggleFullscreen();

        SCREEN_WIDTH = GetScreenWidth();
        SCREEN_HEIGHT = GetScreenHeight();
    }
}

internal void
RotateCameraAroundOrigo(f64 DeltaTime)
{
    Camera3D *Cam = &MainCamera;
    f64 Speed = 10.0f * DeltaTime;
    f64 VerticalSpeed = 5.0f * DeltaTime;

    local_persist f64 Yaw = 45.80f;
    local_persist f64 Pitch = 42.12f;
    local_persist bool CursorEnabled = false;

    local_persist Vector3 direction;
    direction.x = cosf(DEG2RAD * Pitch) * cosf(DEG2RAD * Yaw);
    direction.y = sinf(DEG2RAD * Pitch);
    direction.z = cosf(DEG2RAD * Pitch) * sinf(DEG2RAD * Yaw);
    direction = Vector3Normalize(direction);

    local_persist Vector3 right = Vector3Normalize(Vector3CrossProduct(direction, Cam->up));
    local_persist Vector3 up = Vector3Normalize(Vector3CrossProduct(right, direction));

    if (IsPaused)
    {
        // Disable the cursor to lock it to the center and hide it
        if (CursorEnabled)
        {
            DisableCursor();
            CursorEnabled = false;

            // Set the camera to look at the data from the earths position, roughly
            Cam->position = {4.911170f, -4.564987f, 11.718232f};
            Cam->target = {5.357430f, -3.781510f, 12.150687f};
            direction = {-0.446259f, -0.783477f, -0.432455f};
            Yaw = -136.600;
            Pitch = -51.580;
        }

        // printf("\tFree Look mode\n");
        // printf("\tposition: x=%f, y=%f, z=%f\n", Cam->position.x, Cam->position.y, Cam->position.z);
        // printf("\ttarget: x=%f, y=%f, z=%f\n", Cam->target.x, Cam->target.y, Cam->target.z);
        // printf("\tdirection: x=%f, y=%f, z=%f\n", direction.x, direction.y, direction.z);
        // printf("\tYaw: %f\n", Yaw);
        // printf("\tPitch: %f\n", Pitch);

        Vector2 mouseDelta = GetMouseDelta();

        // Update yaw and pitch based on mouse movement
        Yaw += mouseDelta.x * 0.1f;
        Pitch += mouseDelta.y * 0.1f;

        // Clamp pitch to avoid flipping the camera
        if (Pitch > 89.0f)
            Pitch = 89.0f;
        if (Pitch < -89.0f)
            Pitch = -89.0f;

        // Go slower with LShift
        if (IsKeyDown(KEY_LEFT_SHIFT))
        {
            Speed *= 0.1f;
            VerticalSpeed *= 0.1f;
        }

        // Move camera based on input
        if (IsKeyDown(KEY_W))
        {
            Cam->position = Vector3Subtract(Cam->position, Vector3Scale(direction, Speed));
        }
        if (IsKeyDown(KEY_S))
        {
            Cam->position = Vector3Add(Cam->position, Vector3Scale(direction, Speed));
        }
        if (IsKeyDown(KEY_D))
        {
            Cam->position = Vector3Subtract(Cam->position, Vector3Scale(right, Speed));
        }
        if (IsKeyDown(KEY_A))
        {
            Cam->position = Vector3Add(Cam->position, Vector3Scale(right, Speed));
        }
        if (IsKeyDown(KEY_Q))
        {
            Cam->position = Vector3Subtract(Cam->position, Vector3Scale(up, VerticalSpeed));
        }
        if (IsKeyDown(KEY_E))
        {
            Cam->position = Vector3Add(Cam->position, Vector3Scale(up, VerticalSpeed));
        }

        // Update camera target to reflect the new direction
        Cam->target = Vector3Subtract(Cam->position, direction);
    }
    else
    {
        // If not in free look mode, enable the cursor and restore the original camera logic
        if (!CursorEnabled)
        {
            EnableCursor();
            CursorEnabled = true;
        }

        local_persist f64 PreviousTimeSinceStart = 0.0f;
        PreviousTimeSinceStart += DeltaTime * 0.2f;

        Cam->position.x = 25.0f * cosf(PreviousTimeSinceStart) * Zoom;
        Cam->position.y = 50.0f;
        Cam->position.z = 25.0f * sinf(PreviousTimeSinceStart) * Zoom;

        Cam->target = Vector3Zero();
    }

    // printf("Direction: x=%f, y=%f, z=%f\n", direction.x, direction.y, direction.z);
    // printf("Right:     x=%f, y=%f, z=%f\n", right.x, right.y, right.z);
    // printf("Up:        x=%f, y=%f, z=%f\n", up.x, up.y, up.z);
}

internal void
GameUpdate(f64 DeltaTime)
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

    // @Note(Victor): This is not working as intended, yet
    // if (IsKeyPressed(KEY_FOUR))
    // {
    //     DataToDraw = DRAW_REDSHIFT_DATA;
    // }

    if (IsKeyPressed(KEY_SPACE))
    {
        IsPaused = !IsPaused;
        printf("\tIsPaused: %s\n", IsPaused ? "true" : "false");
    }

    RotateCameraAroundOrigo(DeltaTime);

    f64 Scroll = GetMouseWheelMove();
    if (Scroll != 0.0f)
    {
        const f64 ZoomChange = -2.5f;
        f64 Speed = ZoomChange;
        Zoom = Clamp(Zoom + Scroll * Speed * DeltaTime, 0.0f, 10.0f);
    }
}

internal void
GameRender(f64 DeltaTime)
{
    BeginDrawing();
    ClearBackground(BLACK);

    if (!DataAIsLoaded)
    {
        return;
    }

    // Draw the data around a sphere in 3D
    BeginMode3D(MainCamera);

    /* {
        // Override the projection matrix to set custom near/far planes
        rlMatrixMode(RL_PROJECTION);
        rlLoadIdentity();

        // Set the correct aspect ratio
        f64 aspect = (f64)GetScreenWidth() / (f64)GetScreenHeight();
        f64 nearPlane = 0.1f;
        f64 farPlane = 100000000000.0;

        // Convert fovy from degrees to radians
        f64 fovyRadians = MainCamera.fovy * DEG2RAD;
        f64 top = nearPlane * tan(fovyRadians / 2.0f);
        f64 bottom = -top;
        f64 right = top * aspect;
        f64 left = -right;

        // Set the frustum parameters correctly
        rlFrustum(left, right, bottom, top, nearPlane, farPlane);

        rlMatrixMode(RL_MODELVIEW);
        rlLoadIdentity();
    } */

    DrawSphere({0.0f, 0.0f, 0.0f}, 1.0f, BLUE);

    // Draw the Earth model at the origin (0, 0, 0)
    Vector3 EarthPosition = {0.0f, 0.0f, 0.0f};
    const f64 EarthScale = 1.0f;
    DrawModel(EarthModel, EarthPosition, EarthScale, WHITE);

    // Draw instanced meshes
    if (DataToDraw == DRAW_DATA_A || DataToDraw == DRAW_ALL_DATA)
    {
        Color MyDARKBLUE = {0, 0, 255, 255};
        matInstances.maps[MATERIAL_MAP_DIFFUSE].color = MyDARKBLUE;
        DrawMeshInstanced(SphereMesh, matInstances, MatrixTransformsA, MAX_DATA_POINTS);
    }

    if (DataToDraw == DRAW_DATA_B || DataToDraw == DRAW_ALL_DATA)
    {
        matInstances.maps[MATERIAL_MAP_DIFFUSE].color = RED;
        DrawMeshInstanced(SphereMesh, matInstances, MatrixTransformsB, MAX_DATA_POINTS);
    }

    if (DataToDraw == DRAW_REDSHIFT_DATA)
    {
        matInstances.maps[MATERIAL_MAP_DIFFUSE].color = MAGENTA;
        DrawMeshInstanced(SphereMesh, matInstances, MatrixTransformsRedshift, MAX_REDSHIFT_DATA_POINTS);
    }

    EndMode3D();

    // UI ------------------------------------------------------

    // Draw the FPS with our font
    DrawTextEx(MainFont, TextFormat("FPS: %i", GetFPS()), {10, 10}, 20, 2, WHITE);

    if (!IsPaused)
    {
        // Scroll to zoom
        DrawTextEx(MainFont, TextFormat("Scroll to zoom: %.2f", Zoom), {10, 50}, 16, 2, WHITE);
    }

    // Press F11 to toggle fullscreen
    DrawTextEx(MainFont, TextFormat("Press F11 to toggle fullscreen"), {10, 70}, 16, 2, WHITE);

    // Press 1, 2 or 3 to toggle which data to draw
    DrawTextEx(MainFont, TextFormat("Press 1, 2, 3 or 4 to toggle which data to draw"), {10, 90}, 16, 2, WHITE);

    // Red are uniformly distributed, blue are real data
    DrawTextEx(MainFont, TextFormat("Red are uniformly distributed"), {10, 110}, 16, 2, RED);
    DrawTextEx(MainFont, TextFormat("Blue are real data"), {10, 130}, 16, 2, BLUE);

    // @Note(Victor): This is not working as intended
    // DrawTextEx(MainFont, TextFormat("Magenta are redshift data"), {10, 160}, 16, 2, MAGENTA);

    if (IsPaused)
    {
        DrawTextEx(MainFont, TextFormat("Press W, A, S, D, Q, E to move the camera + Mouse"), {10, 150}, 16, 2, WHITE);
        DrawTextEx(MainFont, TextFormat("Press LShift to move slower"), {10, 170}, 16, 2, WHITE);
    }

    // Press space to pause in the center bottom
    if (IsPaused)
    {
        const f64 TextWidth = MeasureText("Press Space again to go back to Auto Look", 16);
        DrawTextEx(MainFont, TextFormat("Press Space again to go back to Auto Look"), {(float)(SCREEN_WIDTH / 2.0f - (float)TextWidth - 64.0f / 2.0f), (float)SCREEN_HEIGHT - 30.0f}, 16, 2, PURPLE);
    }
    else
    {
        // Highlight the paused text
        const f64 TextWidth = MeasureText("Press Space to enter Free Look mode", 16);
        DrawTextEx(MainFont, "Press Space to enter Free Look mode", {(float)(SCREEN_WIDTH / 2.0f - (float)TextWidth - 64.0f / 2.0f), (float)SCREEN_HEIGHT - 30.0f}, 16, 2, GREEN);
    }

    if (IsPaused)
    {
        const char *IsPausedText = "Free Look";
        const int IsPausedTextLength = strlen(IsPausedText);
        DrawTextEx(MainFont, IsPausedText, {SCREEN_WIDTH - 128.0f - 64.0f, 30}, 20, 2, PURPLE);
    }
    else
    {
        const char *IsPausedText = "Auto Look";
        const int IsPausedTextLength = strlen(IsPausedText);
        DrawTextEx(MainFont, IsPausedText, {SCREEN_WIDTH - 64.0f - 128.0f - 32.0f, 30}, 20, 2, GREEN);
    }

    EndDrawing();
}

internal void
PrintMemoryUsage(void)
{
    printf("\n\tMemory used in GigaBytes: %f\n", (f64)CPUMemory / (f64)Gigabytes(1));
    printf("\tMemory used in MegaBytes: %f\n", (f64)CPUMemory / (f64)Megabytes(1));
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

    free(RedshiftData);
    CPUMemory -= MAX_REDSHIFT_DATA_POINTS * sizeof(ArcminData);
    printf("\n\tFreeing RedshiftData: %lu\n", MAX_REDSHIFT_DATA_POINTS * sizeof(ArcminData));
    PrintMemoryUsage();

    free(MatrixTransformsRedshift);
    CPUMemory -= MAX_REDSHIFT_DATA_POINTS * sizeof(Matrix);
    printf("\n\tFreeing MatrixTransformsRedshift: %lu\n", MAX_REDSHIFT_DATA_POINTS * sizeof(Matrix));
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

internal bool
ReadInputDataFromRedshiftFile(const char *FileName, ArcminData *DataPointsLocation)
{
    // Data format:
    // Name: Galaxy name
    // RA (1950): Right ascension (celestial longitude) in the 1950 epoch (format: HHMMSS.s)
    // DEC: Declination (celestial latitude) in the 1950 epoch (format: DDMMSS)
    // VH/VE/VS: Heliocentric velocity or redshift-related data.
    // Other columns: Additional parameters like magnitude, velocity types, or uncertainties.

    FILE *f = fopen(FileName, "r");
    if (f == NULL)
    {
        printf("Error opening redshift file: %s\n", FileName);
        return false;
    }

    const int bufferSize = 4096;
    char Line[bufferSize]; // Buffer to store each line from the file

    // Skip the header lines (13 lines in this case)
    const int HeaderLines = 13;
    for (int i = 0; i < HeaderLines; ++i)
    {
        if (fgets(Line, sizeof(Line), f) == NULL)
        {
            printf("Error reading header!\n");
            fclose(f);
            return false;
        }
    }

    unsigned long int i = 0;
    while (fgets(Line, sizeof(Line), f) != NULL && i < MAX_REDSHIFT_DATA_POINTS)
    {
        // Remove leading/trailing whitespace (if any)
        char *trimmedLine = strtok(Line, "\n");

        // Skip empty lines
        if (trimmedLine == NULL || strlen(trimmedLine) == 0)
            continue;

        // Tokenize the line assuming space-separated values
        char *Token = strtok(trimmedLine, " ");
        int j = 0;

        while (Token != NULL)
        {
            switch (j)
            {
            case 1: // RA (1950)
                DataPointsLocation[i].right_ascension = atof(Token);
                break;
            case 2: // DEC
                DataPointsLocation[i].declination = atof(Token);
                break;
            case 4: // Redshift (VH)
                DataPointsLocation[i].redshift = atof(Token);
                break;
            default:
                break;
            }

            Token = strtok(NULL, " "); // Continue to the next token
            j++;
        }

        i++;
    }

    if (f != NULL)
    {
        fclose(f);
    }

    printf("\tSuccessfully read %ld redshift data points from %s\n", i, FileName);

    return true;
}

internal bool
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

    RedshiftData = (ArcminData *)calloc(MAX_REDSHIFT_DATA_POINTS, sizeof(ArcminData));

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

    if (ReadInputDataFromRedshiftFile(RedshiftDataFilename, RedshiftData)) // or another appropriate data structure
    {
        printf("\tSuccessfully loaded redshift data from %s\n", RedshiftDataFilename);
        CPUMemory += MAX_REDSHIFT_DATA_POINTS * sizeof(ArcminData);
    }
    else
    {
        printf("Failed to load redshift data from %s\n", RedshiftDataFilename);
        CleanupOurStuff();
        return 1;
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
    // MainCamera.position = {0.0f, 60.0f, 100.0f};
    MainCamera.position = {0.0f, 0.0f, 0.0f};
    MainCamera.target = {0.0f, 0.0f, 0.0f};
    MainCamera.up = {0.0f, 1.0f, 0.0f};
    MainCamera.fovy = 65.0f; // Adjust if necessary
    MainCamera.projection = CAMERA_PERSPECTIVE;

    // Define transforms to be uploaded to GPU for instances
    {
        MatrixTransformsA = (Matrix *)calloc(MAX_DATA_POINTS, sizeof(Matrix));
        CPUMemory += MAX_DATA_POINTS * sizeof(Matrix);

        MatrixTransformsB = (Matrix *)calloc(MAX_DATA_POINTS, sizeof(Matrix));
        CPUMemory += MAX_DATA_POINTS * sizeof(Matrix);

        MatrixTransformsRedshift = (Matrix *)calloc(MAX_REDSHIFT_DATA_POINTS, sizeof(Matrix));
        CPUMemory += MAX_REDSHIFT_DATA_POINTS * sizeof(Matrix);

        Matrix rotation = MatrixRotateXYZ({0.0f, 0.0f, 0.0f});

        for (unsigned long int i = 0; i < MAX_DATA_POINTS; ++i)
        {
            // DataPointsA real galaxies
            {
                // Transform the arc minutes into radians that the trigonometric functions take as input. (sinf, cosf, tanf)
                f64 RightAscensionRad = (DataPointsA[i].right_ascension / 60.0f) * PIdividedBy180;
                f64 DeclinationRad = (DataPointsA[i].declination / 60.0f) * PIdividedBy180;

                // Calculate the position on the sphere using spherical coordinates
                f64 Radius = 50.0f;
                f64 X = Radius * cosf(RightAscensionRad) * cosf(DeclinationRad);
                f64 Y = Radius * sinf(DeclinationRad);
                f64 Z = Radius * sinf(RightAscensionRad) * cosf(DeclinationRad);

                // Create a model matrix for each data point to position it
                MatrixTransformsA[i] = MatrixIdentity();
                MatrixTransformsA[i] = MatrixMultiply(MatrixTransformsA[i], MatrixScale(0.1f, 0.1f, 0.1f));
                MatrixTransformsA[i] = MatrixMultiply(MatrixTransformsA[i], MatrixTranslate(X, Y, Z));
            }

            // DataPointsB uniformly distributed (galaxies)
            {
                f64 RightAscensionRad = (DataPointsB[i].right_ascension / 60.0f) * PIdividedBy180;
                f64 DeclinationRad = (DataPointsB[i].declination / 60.0f) * PIdividedBy180;

                // Calculate the position on the sphere using spherical coordinates
                f64 Radius = 50.0f;
                f64 X = Radius * cosf(RightAscensionRad) * cosf(DeclinationRad);
                f64 Y = Radius * sinf(DeclinationRad);
                f64 Z = Radius * sinf(RightAscensionRad) * cosf(DeclinationRad);

                // Create a model matrix for each data point to position it
                MatrixTransformsB[i] = MatrixIdentity();
                MatrixTransformsB[i] = MatrixMultiply(MatrixTransformsB[i], MatrixScale(0.1f, 0.1f, 0.1f));
                MatrixTransformsB[i] = MatrixMultiply(MatrixTransformsB[i], MatrixTranslate(X, Y, Z));
            }
        } // end of for loop for the course data

        // Redshift data points with distance from the earth
        // Redshift can be mapped to a distance value in megaparsecs (Mpc) or another suitable unit for distance.
        // Assuming Redshift has already been scaled to represent the distance directly, we use it as the radius.
        for (unsigned long int i = 0; i < MAX_REDSHIFT_DATA_POINTS; ++i)
        {
            // Convert RA and DEC to radians
            f64 rightAscensionRad = (RedshiftData[i].right_ascension / 60.0f) * PIdividedBy180;
            f64 declinationRad = (RedshiftData[i].declination / 60.0f) * PIdividedBy180;

            // Convert redshift to distance in Megaparsecs
            f64 distanceMpc = RedshiftToDistance(RedshiftData[i].redshift);

            // Convert distance to some meaningful scale for your simulation
            // For example, if you want to work in parsecs instead of megaparsecs:
            f64 distance = distanceMpc * hubbleConstant; // Convert Mpc to parsecs

            // Calculate the position in 3D space using spherical to Cartesian conversion
            f64 X = distance * cos(declinationRad) * cos(rightAscensionRad);
            f64 Y = distance * cos(declinationRad) * sin(rightAscensionRad);
            f64 Z = distance * sin(declinationRad);

            // Apply this position to your model matrix (for example)
            MatrixTransformsRedshift[i] = MatrixIdentity();
            MatrixTransformsRedshift[i] = MatrixMultiply(MatrixTransformsRedshift[i], MatrixScale(10000.0f, 10000.0f, 10000.0f));
            MatrixTransformsRedshift[i] = MatrixMultiply(MatrixTransformsRedshift[i], MatrixTranslate(X, Y, Z));
        }
    }

    // Raylib
    {
        SetTraceLogLevel(LOG_WARNING);
        SetConfigFlags(FLAG_WINDOW_RESIZABLE);
        InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "galaxy_visuazation_raylib");

#if defined(PLATFORM_WEB)
        emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
        SetTargetFPS(60);
    }

    MainFont = LoadFontEx("./resources/fonts/SuperMarioBros2.ttf", 32, 0, 250);
    CustomShader = LoadShader("./shaders/lighting_instancing.vs", "./shaders/lighting.fs");
    SphereMesh = GenMeshSphere(0.2f, 16, 16);

    // Get shader locations
    CustomShader.locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(CustomShader, "mvp");
    CustomShader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(CustomShader, "viewPos");
    CustomShader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocationAttrib(CustomShader, "instanceTransform");

    // Lighting
    {
        // Setting shader values
        i32 AmbientLoc = GetShaderLocation(CustomShader, "ambient");
        f64 AmbientValue[4] = {1.0, 1.0, 1.0, 1.0};
        SetShaderValue(CustomShader, AmbientLoc, &AmbientValue, SHADER_UNIFORM_VEC4);

        i32 ColorDiffuseLoc = GetShaderLocation(CustomShader, "colorDiffuse");
        f64 DiffuseValue[4] = {1.0, 1.0, 1.0, 1.0};
        SetShaderValue(CustomShader, ColorDiffuseLoc, &DiffuseValue, SHADER_UNIFORM_VEC4);

        // Like the sun shining on the earth
        CreateLight(LIGHT_DIRECTIONAL, {1000.0f, 1000.0f, 0.0f}, Vector3Zero(), WHITE, CustomShader);

        // @Note(Victor): We can add more lights to the scene to better show the colors of the galaxies
        CreateLight(LIGHT_DIRECTIONAL, {-1000.0f, -1000.0f, 0.0f}, Vector3Zero(), WHITE, CustomShader);
        CreateLight(LIGHT_DIRECTIONAL, {0.0f, 0.0f, 1000.0f}, Vector3Zero(), WHITE, CustomShader);
        CreateLight(LIGHT_DIRECTIONAL, {0.0f, 0.0f, -1000.0f}, Vector3Zero(), WHITE, CustomShader);

        // We can also add a point light at the center of the earth
        CreateLight(LIGHT_POINT, {0.0f, 0.0f, 0.0f}, Vector3Zero(), WHITE, CustomShader);
    }

    // Material
    {
        // NOTE: We are assigning the intancing shader to material.shader
        // to be used on mesh drawing with DrawMeshInstanced()
        Material GalaxyMaterial = LoadMaterialDefault();
        GalaxyMaterial.shader = CustomShader;

        GalaxyMaterial.maps[MATERIAL_MAP_DIFFUSE].texture = LoadTexture("./resources/images/galaxy_test_texture_diffuse.png");
        GalaxyMaterial.maps[MATERIAL_MAP_SPECULAR].texture = LoadTexture("./resources/images/galaxy_test_texture_specular.png");

        GalaxyMaterial.maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
        GalaxyMaterial.maps[MATERIAL_MAP_SPECULAR].value = 1.0f;

        // Set the shininess for specular reflections
        float shininess = 32.0f;
        SetShaderValue(GalaxyMaterial.shader, GetShaderLocation(GalaxyMaterial.shader, "shininess"), &shininess, SHADER_UNIFORM_FLOAT);

        matInstances = GalaxyMaterial;
        matInstances.shader = CustomShader;
        matInstances.maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
    }

    printf("\n\tMemory usage before we start the game loop\n");
    PrintMemoryUsage();

    // Load the Earth model
    EarthModel = LoadModel("./resources/Earth_1_12756.glb");

    // Optionally, you can scale the model if needed
    Matrix scaleMatrix = MatrixScale(0.01f, 0.01f, 0.01f);
    EarthModel.transform = MatrixMultiply(EarthModel.transform, scaleMatrix);

    // Main loop
    while (!WindowShouldClose()) // Detect window close button or ESC key
    {
        f64 DeltaTime = GetFrameTime();
        GameUpdate(DeltaTime);
        GameRender(DeltaTime);
    }
#endif
        CleanupOurStuff();

        return (0);
    }
