// Defines -----------------------------------------------------------------------
#define RLIGHTS_IMPLEMENTATION

// Includes ----------------------------------------------------------------------
#include "includes.h"
#include "raylib_includes.h"

#define SCREEN_WIDTH = 640 * 2
#define SCREEN_HEIGHT = 360 * 2

// Types -------------------------------------------------------------------------
typedef struct
{
    f64 right_ascension;
    f64 declination;
    f64 redshift;
} arcmin_data_t;

typedef enum
{
    // Draw the data from the files from the ÅA course
    DRAW_DATA_A,
    DRAW_DATA_B,
    DRAW_DATA_ALL,

    // Draw the redshift data, not from the course
    DRAW_DATA_REDSHIFT,

    DRAW_DATA_COUNT
} draw_data_t;

typedef struct
{
    u64 CPU_memory_allocated;

    arcmin_data_t arcmin_data;
    draw_data_t draw_data;

    // @Note(Victor): Data from the course, only celestial coordinates, no redshift (distance)
    arcmin_data_t *data_points_a;
    arcmin_data_t *data_points_b;

    // @Note(Victor): Data from the redshift file with the appriximated distances to the galaxies
    arcmin_data_t *redshift_data;

    bool debug;
    bool data_is_loaded;
    bool is_paused;

    Font main_font;

    i32 window_width;
    i32 window_height;
    Camera3D main_camera;
    f64 zoom;
    Shader custom_shader;

    Mesh sphere_mesh;
    Model earth_model;

    // Batch rendering in Raylib with a custom shader
    Material material_instance;
    Matrix *matrix_transforms_a;
    Matrix *matrix_transforms_b;
    Matrix *matrix_transforms_redshift;

    draw_data_t data_to_draw;

} app_state_t;

// Variables ---------------------------------------------------------------------

const f64 PI_by_180 = (PI / 180.0);
const char *data_a_filename = "./input_data/data_100k_arcmin.txt";
const char *data_b_filename = "./input_data/flat_100k_arcmin.txt";
const char *redshift_data_filename = "./redshift_input_data/seyfert.dat";

const unsigned long int MAX_DATA_POINTS = 100000UL;
const unsigned long int MAX_REDSHIFT_DATA_POINTS = 100000UL; // @Note(Victor): This is set when we read the redshift data

// Assuming speed of light in km/s for converting redshift to distance (simplified calculation)
const f64 speed_of_light_kmh = 299792.458; // Speed of light in km/s
const f64 hubble_constant = 70.0;          // Hubble constant in km/s/Mpc

// Redshift data calculations
// ----------------------------------------------------------------------------------
// Function to convert RA from HHMMSS to degrees
// internal f64
// ConvertRaToDegrees(f64 raHHMMSS)
// {
//     int hours = (int)(raHHMMSS / 10000);
//     int minutes = (int)((raHHMMSS - (hours * 10000)) / 100);
//     f64 seconds = raHHMMSS - (hours * 10000) - (minutes * 100);
//
//     return 15.0 * (hours + (minutes / 60.0) + (seconds / 3600.0)); // 1 hour = 15 degrees
// }

// Function to convert DEC from DDMMSS to degrees
// DEC: Declination
// DDMMSS: Degrees, minutes, seconds
// internal f64
// ConvertDecToDegrees(f64 decDDMMSS)
// {
//     int degrees = (int)(decDDMMSS / 10000);
//     int minutes = (int)((decDDMMSS - (degrees * 10000)) / 100);
//     f64 seconds = decDDMMSS - (degrees * 10000) - (minutes * 100);
//
//     f64 decDegrees = abs(degrees) + (minutes / 60.0) + (seconds / 3600.0);
//     return (degrees < 0) ? -decDegrees : decDegrees;
// }

internal f64
redshift_to_distance(f64 redshift)
{
    // Distance in Megaparsecs (Mpc)
    return (speed_of_light_kmh * redshift) / hubble_constant;
}

// Convert spherical coordinates (RA, Dec, distance) to Cartesian (X, Y, Z)
// internal void
// CalculatePosition(f64 ra, f64 dec, f64 redshift, f64 *X, f64 *Y, f64 *Z)
// {
//     f64 distance = redshift_to_distance(redshift); // Convert redshift to distance (Mpc)
//
//     // Convert degrees to radians
//     f64 raRad = ra * PI_by_180;
//     f64 decRad = dec * PI_by_180;
//
//     // Calculate Cartesian coordinates
//     *X = distance * cos(decRad) * cos(raRad);
//     *Y = distance * cos(decRad) * sin(raRad);
//     *Z = distance * sin(decRad);
// }
// ----------------------------------------------------------------------------------

internal void
parse_input_args(app_state_t *app_state, i32 argc, char **argv)
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
            printf("\tRunning in DEBUG mode!\n");
            app_state->debug = true;
        }
    }
}

internal void
handle_window_resize(app_state_t *app_state)
{
    if (IsWindowResized() && !IsWindowFullscreen())
    {
        app_state->window_width = GetScreenWidth();
        app_state->window_height = GetScreenHeight();
    }

    if (IsKeyPressed(KEY_ENTER) && (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)))
    {
        i32 Display = GetCurrentMonitor();

        if (IsWindowFullscreen())
        {
            // If we are full screen, then go back to the windowed size
            SetWindowSize(app_state->window_width, app_state->window_height);
        }
        else
        {
            // If we are not full screen, set the window size to match the monitor we are on
            SetWindowSize(GetMonitorWidth(Display), GetMonitorHeight(Display));
        }

        ToggleFullscreen();

        app_state->window_width = GetScreenWidth();
        app_state->window_height = GetScreenHeight();
    }
}

internal void
rotate_camera_around_origo(app_state_t *app_state, f64 dt)
{
    Camera3D *Cam = &app_state->main_camera;
    f64 Speed = 10.0f * dt;
    f64 VerticalSpeed = 5.0f * dt;

    local_persist f64 Yaw = 45.80f;
    local_persist f64 Pitch = 42.12f;
    local_persist bool CursorEnabled = false;

    local_persist Vector3 direction;
    direction.x = cosf(DEG2RAD * Pitch) * cosf(DEG2RAD * Yaw);
    direction.y = sinf(DEG2RAD * Pitch);
    direction.z = cosf(DEG2RAD * Pitch) * sinf(DEG2RAD * Yaw);
    direction = Vector3Normalize(direction);

    Vector3 right = Vector3Normalize(Vector3CrossProduct(direction, Cam->up));
    Vector3 up = Vector3Normalize(Vector3CrossProduct(right, direction));

    if (app_state->is_paused)
    {
        // Disable the cursor to lock it to the center and hide it
        if (CursorEnabled)
        {
            DisableCursor();
            CursorEnabled = false;

            // Set the camera to look at the data from the earths position, roughly
            Cam->position = (Vector3){4.911170f, -4.564987f, 11.718232f};
            Cam->target = (Vector3){5.357430f, -3.781510f, 12.150687f};
            direction = (Vector3){-0.446259f, -0.783477f, -0.432455f};
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
        PreviousTimeSinceStart += dt * 0.2f;

        Cam->position.x = 25.0f * cosf(PreviousTimeSinceStart) * app_state->zoom;
        Cam->position.y = 50.0f;
        Cam->position.z = 25.0f * sinf(PreviousTimeSinceStart) * app_state->zoom;

        Cam->target = Vector3Zero();
    }

    // printf("Direction: x=%f, y=%f, z=%f\n", direction.x, direction.y, direction.z);
    // printf("Right:     x=%f, y=%f, z=%f\n", right.x, right.y, right.z);
    // printf("Up:        x=%f, y=%f, z=%f\n", up.x, up.y, up.z);
}

internal void
game_update(app_state_t *app_state, f64 dt)
{
    handle_window_resize(app_state);

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
        app_state->data_to_draw = DRAW_DATA_A;
    }

    if (IsKeyPressed(KEY_TWO))
    {
        app_state->data_to_draw = DRAW_DATA_B;
    }

    if (IsKeyPressed(KEY_THREE))
    {
        app_state->data_to_draw = DRAW_DATA_ALL;
    }

    // @Note(Victor): This is not working as intended, yet
    // if (IsKeyPressed(KEY_FOUR))
    // {
    //     data_to_draw = DRAW_DATA_REDSHIFT;
    // }

    if (IsKeyPressed(KEY_SPACE))
    {
        app_state->is_paused = !app_state->is_paused;
        printf("\tIsPaused: %s\n", app_state->is_paused ? "true" : "false");
    }

    rotate_camera_around_origo(app_state, dt);

    f64 Scroll = GetMouseWheelMove();
    if (Scroll != 0.0f)
    {
        const f64 ZoomChange = -2.5f;
        f64 Speed = ZoomChange;
        app_state->zoom = Clamp(app_state->zoom + Scroll * Speed * dt, 0.0f, 10.0f);
    }
}

internal void
game_render(app_state_t *app_state, f64 dt)
{
    (void)dt;

    BeginDrawing();
    ClearBackground(BLACK);

    if (!app_state->data_is_loaded)
    {
        return;
    }

    // Draw the data around a sphere in 3D
    BeginMode3D(app_state->main_camera);

    /* {
        // Override the projection matrix to set custom near/far planes
        rlMatrixMode(RL_PROJECTION);
        rlLoadIdentity();

        // Set the correct aspect ratio
        f64 aspect = (f64)GetScreenWidth() / (f64)GetScreenHeight();
        f64 nearPlane = 0.1f;
        f64 farPlane = 100000000000.0;

        // Convert fovy from degrees to radians
        f64 fovyRadians = main_camera.fovy * DEG2RAD;
        f64 top = nearPlane * tan(fovyRadians / 2.0f);
        f64 bottom = -top;
        f64 right = top * aspect;
        f64 left = -right;

        // Set the frustum parameters correctly
        rlFrustum(left, right, bottom, top, nearPlane, farPlane);

        rlMatrixMode(RL_MODELVIEW);
        rlLoadIdentity();
    } */

    DrawSphere((Vector3){0.0f, 0.0f, 0.0f}, 1.0f, BLUE);

    // Draw the Earth model at the origin (0, 0, 0)
    Vector3 EarthPosition = {0.0f, 0.0f, 0.0f};
    const f64 EarthScale = 1.0f;
    DrawModel(app_state->earth_model, EarthPosition, EarthScale, WHITE);

    // Draw instanced meshes
    if (app_state->data_to_draw == DRAW_DATA_A || app_state->data_to_draw == DRAW_DATA_ALL)
    {
        Color MyDARKBLUE = {0, 0, 255, 255};
        app_state->material_instance.maps[MATERIAL_MAP_DIFFUSE].color = MyDARKBLUE;
        DrawMeshInstanced(app_state->sphere_mesh, app_state->material_instance, app_state->matrix_transforms_a, MAX_DATA_POINTS);
    }

    if (app_state->data_to_draw == DRAW_DATA_B || app_state->data_to_draw == DRAW_DATA_ALL)
    {
        app_state->material_instance.maps[MATERIAL_MAP_DIFFUSE].color = RED;
        DrawMeshInstanced(app_state->sphere_mesh, app_state->material_instance, app_state->matrix_transforms_b, MAX_DATA_POINTS);
    }

    if (app_state->data_to_draw == DRAW_DATA_REDSHIFT)
    {
        app_state->material_instance.maps[MATERIAL_MAP_DIFFUSE].color = MAGENTA;
        DrawMeshInstanced(app_state->sphere_mesh, app_state->material_instance, app_state->matrix_transforms_redshift, MAX_REDSHIFT_DATA_POINTS);
    }

    EndMode3D();

    // UI ------------------------------------------------------

    // Draw the FPS with our font
    DrawTextEx(app_state->main_font, TextFormat("FPS: %i", GetFPS()), (Vector2){10, 10}, 20, 2, WHITE);

    if (!app_state->is_paused)
    {
        // Scroll to zoom
        DrawTextEx(app_state->main_font, TextFormat("Scroll to zoom: %.2f", app_state->zoom), (Vector2){10, 50}, 16, 2, WHITE);
    }

    // Press F11 to toggle fullscreen
    DrawTextEx(app_state->main_font, TextFormat("Press F11 to toggle fullscreen"), (Vector2){10, 70}, 16, 2, WHITE);

    // Press 1, 2 or 3 to toggle which data to draw
    DrawTextEx(app_state->main_font, TextFormat("Press 1, 2, 3 or 4 to toggle which data to draw"), (Vector2){10, 90}, 16, 2, WHITE);

    // Red are uniformly distributed, blue are real data
    DrawTextEx(app_state->main_font, TextFormat("Red are uniformly distributed"), (Vector2){10, 110}, 16, 2, RED);
    DrawTextEx(app_state->main_font, TextFormat("Blue are real data"), (Vector2){10, 130}, 16, 2, BLUE);

    // @Note(Victor): This is not working as intended
    // DrawTextEx(main_font, TextFormat("Magenta are redshift data"), {10, 160}, 16, 2, MAGENTA);

    if (app_state->is_paused)
    {
        DrawTextEx(app_state->main_font, TextFormat("Press W, A, S, D, Q, E to move the camera + Mouse"), (Vector2){10, 150}, 16, 2, WHITE);
        DrawTextEx(app_state->main_font, TextFormat("Press LShift to move slower"), (Vector2){10, 170}, 16, 2, WHITE);
    }

    // Press space to pause in the center bottom
    if (app_state->is_paused)
    {
        const f64 TextWidth = MeasureText("Press Space again to go back to Auto Look", 16);
        DrawTextEx(app_state->main_font, TextFormat("Press Space again to go back to Auto Look"), (Vector2){(float)(app_state->window_width / 2.0f - (float)TextWidth - 64.0f / 2.0f), (float)app_state->window_height - 30.0f}, 16, 2, PURPLE);
    }
    else
    {
        // Highlight the paused text
        const f64 TextWidth = MeasureText("Press Space to enter Free Look mode", 16);
        DrawTextEx(app_state->main_font, "Press Space to enter Free Look mode", (Vector2){(float)(app_state->window_width / 2.0f - (float)TextWidth - 64.0f / 2.0f), (float)app_state->window_height - 30.0f}, 16, 2, GREEN);
    }

    if (app_state->is_paused)
    {
        const char *IsPausedText = "Free Look";
        // const int IsPausedTextLength = strlen(IsPausedText);
        DrawTextEx(app_state->main_font, IsPausedText, (Vector2){app_state->window_width - 128.0f - 64.0f, 30}, 20, 2, PURPLE);
    }
    else
    {
        const char *IsPausedText = "Auto Look";
        // const int IsPausedTextLength = strlen(IsPausedText);
        DrawTextEx(app_state->main_font, IsPausedText, (Vector2){app_state->window_width - 64.0f - 128.0f - 32.0f, 30}, 20, 2, GREEN);
    }

    EndDrawing();
}

internal void
print_memory_usage(app_state_t *app_state)
{
    printf("\n\tMemory used in GigaBytes: %f\n", (f64)app_state->CPU_memory_allocated / (f64)Gigabytes(1));
    printf("\tMemory used in MegaBytes: %f\n", (f64)app_state->CPU_memory_allocated / (f64)Megabytes(1));
}

internal void
cleanup(app_state_t *app_state)
{
    CloseWindow(); // Close window and OpenGL context
    printf("\n\tClosed window and OpenGL context\n");

    free(app_state->data_points_a);
    app_state->CPU_memory_allocated -= MAX_DATA_POINTS * sizeof(arcmin_data_t);
    printf("\n\tFreeing data_points_a %lu\n", MAX_DATA_POINTS * sizeof(arcmin_data_t));
    print_memory_usage(app_state);

    free(app_state->data_points_b);
    app_state->CPU_memory_allocated -= MAX_DATA_POINTS * sizeof(arcmin_data_t);
    printf("\n\tFreeing data_points_b: %lu\n", MAX_DATA_POINTS * sizeof(arcmin_data_t));
    print_memory_usage(app_state);

    free(app_state->matrix_transforms_a);
    app_state->CPU_memory_allocated -= MAX_DATA_POINTS * sizeof(Matrix);
    printf("\n\tFreeing matrix_transforms_a: %lu\n", MAX_DATA_POINTS * sizeof(Matrix));
    print_memory_usage(app_state);

    free(app_state->matrix_transforms_b);
    app_state->CPU_memory_allocated -= MAX_DATA_POINTS * sizeof(Matrix);
    printf("\n\tFreeing matrix_transforms_b: %lu\n", MAX_DATA_POINTS * sizeof(Matrix));
    print_memory_usage(app_state);

    free(app_state->redshift_data);
    app_state->CPU_memory_allocated -= MAX_REDSHIFT_DATA_POINTS * sizeof(arcmin_data_t);
    printf("\n\tFreeing redshift_data: %lu\n", MAX_REDSHIFT_DATA_POINTS * sizeof(arcmin_data_t));
    print_memory_usage(app_state);

    free(app_state->matrix_transforms_redshift);
    app_state->CPU_memory_allocated -= MAX_REDSHIFT_DATA_POINTS * sizeof(Matrix);
    printf("\n\tFreeing matrix_transforms_redshift: %lu\n", MAX_REDSHIFT_DATA_POINTS * sizeof(Matrix));
    print_memory_usage(app_state);

    // @Note(Victor): There should be no allocated memory left
    Assert(app_state->CPU_memory_allocated == 0);
}

// internal void
// sig_int_handler(app_state_t *app_state, i32 Signal)
// {
//     (void)Signal;
//
//     printf("\tCaught SIGINT, exiting peacefully!\n");
//
//     cleanup(app_state);
//
//     exit(0);
// }

internal bool
read_input_data_from_redshift_file(const char *FileName, arcmin_data_t *DataPointsLocation)
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
read_input_data_from_file(const char *FileName, arcmin_data_t *DataPointsLocation)
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
    // signal(SIGINT, sig_int_handler);

    app_state_t *app_state = (app_state_t *)calloc(1, sizeof(app_state_t));
    if (app_state == NULL)
    {
        printf("Error allocating memory for app_state!\n");
        return (1);
    }

    parse_input_args(app_state, argc, argv);

    app_state->CPU_memory_allocated = 0L;
    app_state->debug = false;
    app_state->data_is_loaded = false;
    app_state->is_paused = false;

    app_state->data_points_a = NULL;
    app_state->data_points_b = NULL;
    app_state->redshift_data = NULL;

    app_state->main_camera = (Camera3D){0};
    app_state->zoom = 1.0f * PI;
    app_state->custom_shader = (Shader){0};

    app_state->material_instance = (Material){0};
    app_state->sphere_mesh = (Mesh){0};
    app_state->earth_model = (Model){0};

    app_state->matrix_transforms_a = NULL;
    app_state->matrix_transforms_b = NULL;
    app_state->matrix_transforms_redshift = NULL;

    app_state->data_to_draw = DRAW_DATA_ALL;

    app_state->data_points_a = (arcmin_data_t *)calloc(MAX_DATA_POINTS, sizeof(arcmin_data_t));
    if (app_state->data_points_a == NULL)
    {
        printf("Error allocating memory for data_points_a!\n");
        free(app_state);
        return (1);
    }
    app_state->CPU_memory_allocated += MAX_DATA_POINTS * sizeof(arcmin_data_t);

    app_state->data_points_b = (arcmin_data_t *)calloc(MAX_DATA_POINTS, sizeof(arcmin_data_t));
    if (app_state->data_points_b == NULL)
    {
        printf("Error allocating memory for data_points_b!\n");
        free(app_state->data_points_a);
        free(app_state);
        return (1);
    }
    app_state->CPU_memory_allocated += MAX_DATA_POINTS * sizeof(arcmin_data_t);

    app_state->redshift_data = (arcmin_data_t *)calloc(MAX_REDSHIFT_DATA_POINTS, sizeof(arcmin_data_t));

    if (read_input_data_from_file(data_a_filename, app_state->data_points_a))
    {
        printf("\tReadInputDataFromFile: %s succeeded!\n", data_a_filename);
    }
    else
    {
        printf("\tReadInputDataFromFile: %s failed!\n", data_a_filename);
        cleanup(app_state);
        return (1);
    }

    if (read_input_data_from_file(data_b_filename, app_state->data_points_b))
    {
        printf("\tReadInputDataFromFile: %s succeeded!\n", data_b_filename);
    }
    else
    {
        printf("\tReadInputDataFromFile: %s failed!\n", data_b_filename);
        cleanup(app_state);
        return (1);
    }

    if (read_input_data_from_redshift_file(redshift_data_filename, app_state->redshift_data)) // or another appropriate data structure
    {
        printf("\tSuccessfully loaded redshift data from %s\n", redshift_data_filename);
        app_state->CPU_memory_allocated += MAX_REDSHIFT_DATA_POINTS * sizeof(arcmin_data_t);
    }
    else
    {
        printf("Failed to load redshift data from %s\n", redshift_data_filename);
        cleanup(app_state);
        return 1;
    }

    printf("\tHello from raylib_galaxy_application!\n\n");

    unsigned long int Count = 0;
    for (unsigned long int i = 0; i < MAX_DATA_POINTS; ++i)
    {
        if (app_state->data_points_a[i].right_ascension != 0.0f)
        {
            Count++;
        }
    }

    Assert(Count == MAX_DATA_POINTS);
    Assert(app_state->data_points_b != NULL);

    Count = 0;
    for (unsigned long int i = 0; i < MAX_DATA_POINTS; ++i)
    {
        if (app_state->data_points_b[i].right_ascension != 0.0f)
        {
            Count++;
        }
    }

    Assert(Count == MAX_DATA_POINTS);

    app_state->data_is_loaded = true;

    // Set the camera to rotate around the center of the data
    // main_camera.position = {0.0f, 60.0f, 100.0f};
    app_state->main_camera.position = (Vector3){0.0f, 0.0f, 0.0f};
    app_state->main_camera.target = (Vector3){0.0f, 0.0f, 0.0f};
    app_state->main_camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    app_state->main_camera.fovy = 65.0f; // Adjust if necessary
    app_state->main_camera.projection = CAMERA_PERSPECTIVE;

    // Define transforms to be uploaded to GPU for instances
    {
        app_state->matrix_transforms_a = (Matrix *)calloc(MAX_DATA_POINTS, sizeof(Matrix));
        app_state->CPU_memory_allocated += MAX_DATA_POINTS * sizeof(Matrix);

        app_state->matrix_transforms_b = (Matrix *)calloc(MAX_DATA_POINTS, sizeof(Matrix));
        app_state->CPU_memory_allocated += MAX_DATA_POINTS * sizeof(Matrix);

        app_state->matrix_transforms_redshift = (Matrix *)calloc(MAX_REDSHIFT_DATA_POINTS, sizeof(Matrix));
        app_state->CPU_memory_allocated += MAX_REDSHIFT_DATA_POINTS * sizeof(Matrix);

        // Matrix rotation = MatrixRotateXYZ((Vector3){0.0f, 0.0f, 0.0f});

        for (unsigned long int i = 0; i < MAX_DATA_POINTS; ++i)
        {
            // data_points_a real galaxies
            {
                // Transform the arc minutes into radians that the trigonometric functions take as input. (sinf, cosf, tanf)
                f64 RightAscensionRad = (app_state->data_points_a[i].right_ascension / 60.0f) * PI_by_180;
                f64 DeclinationRad = (app_state->data_points_a[i].declination / 60.0f) * PI_by_180;

                // Calculate the position on the sphere using spherical coordinates
                f64 Radius = 50.0f;
                f64 X = Radius * cosf(RightAscensionRad) * cosf(DeclinationRad);
                f64 Y = Radius * sinf(DeclinationRad);
                f64 Z = Radius * sinf(RightAscensionRad) * cosf(DeclinationRad);

                // Create a model matrix for each data point to position it
                app_state->matrix_transforms_a[i] = MatrixIdentity();
                app_state->matrix_transforms_a[i] = MatrixMultiply(app_state->matrix_transforms_a[i], MatrixScale(0.1f, 0.1f, 0.1f));
                app_state->matrix_transforms_a[i] = MatrixMultiply(app_state->matrix_transforms_a[i], MatrixTranslate(X, Y, Z));
            }

            // data_points_b uniformly distributed (galaxies)
            {
                f64 RightAscensionRad = (app_state->data_points_b[i].right_ascension / 60.0f) * PI_by_180;
                f64 DeclinationRad = (app_state->data_points_b[i].declination / 60.0f) * PI_by_180;

                // Calculate the position on the sphere using spherical coordinates
                f64 Radius = 50.0f;
                f64 X = Radius * cosf(RightAscensionRad) * cosf(DeclinationRad);
                f64 Y = Radius * sinf(DeclinationRad);
                f64 Z = Radius * sinf(RightAscensionRad) * cosf(DeclinationRad);

                // Create a model matrix for each data point to position it
                app_state->matrix_transforms_b[i] = MatrixIdentity();
                app_state->matrix_transforms_b[i] = MatrixMultiply(app_state->matrix_transforms_b[i], MatrixScale(0.1f, 0.1f, 0.1f));
                app_state->matrix_transforms_b[i] = MatrixMultiply(app_state->matrix_transforms_b[i], MatrixTranslate(X, Y, Z));
            }
        } // end of for loop for the course data

        // Redshift data points with distance from the earth
        // Redshift can be mapped to a distance value in megaparsecs (Mpc) or another suitable unit for distance.
        // Assuming Redshift has already been scaled to represent the distance directly, we use it as the radius.
        for (unsigned long int i = 0; i < MAX_REDSHIFT_DATA_POINTS; ++i)
        {
            // Convert RA and DEC to radians
            f64 rightAscensionRad = (app_state->redshift_data[i].right_ascension / 60.0f) * PI_by_180;
            f64 declinationRad = (app_state->redshift_data[i].declination / 60.0f) * PI_by_180;

            // Convert redshift to distance in Megaparsecs
            f64 distanceMpc = redshift_to_distance(app_state->redshift_data[i].redshift);

            // Convert distance to some meaningful scale for your simulation
            // For example, if you want to work in parsecs instead of megaparsecs:
            f64 distance = distanceMpc * hubble_constant; // Convert Mpc to parsecs

            // Calculate the position in 3D space using spherical to Cartesian conversion
            f64 X = distance * cos(declinationRad) * cos(rightAscensionRad);
            f64 Y = distance * cos(declinationRad) * sin(rightAscensionRad);
            f64 Z = distance * sin(declinationRad);

            // Apply this position to your model matrix (for example)
            app_state->matrix_transforms_redshift[i] = MatrixIdentity();
            app_state->matrix_transforms_redshift[i] = MatrixMultiply(app_state->matrix_transforms_redshift[i], MatrixScale(10000.0f, 10000.0f, 10000.0f));
            app_state->matrix_transforms_redshift[i] = MatrixMultiply(app_state->matrix_transforms_redshift[i], MatrixTranslate(X, Y, Z));
        }
    }

    // Raylib
    {
        SetTraceLogLevel(LOG_WARNING);
        SetConfigFlags(FLAG_WINDOW_RESIZABLE);
        InitWindow(app_state->window_width, app_state->window_height, "galaxy_visuazation_raylib");
        SetTargetFPS(60);
    }

    app_state->main_font = LoadFontEx("./resources/fonts/SuperMarioBros2.ttf", 32, 0, 250);
    app_state->custom_shader = LoadShader("./shaders/lighting_instancing.vs", "./shaders/lighting.fs");
    app_state->sphere_mesh = GenMeshSphere(0.2f, 16, 16);

    // Get shader locations
    app_state->custom_shader.locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(app_state->custom_shader, "mvp");
    app_state->custom_shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(app_state->custom_shader, "viewPos");
    app_state->custom_shader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocationAttrib(app_state->custom_shader, "instanceTransform");

    // Lighting
    {
        // Setting shader values
        i32 AmbientLoc = GetShaderLocation(app_state->custom_shader, "ambient");
        f64 AmbientValue[4] = {1.0, 1.0, 1.0, 1.0};
        SetShaderValue(app_state->custom_shader, AmbientLoc, &AmbientValue, SHADER_UNIFORM_VEC4);

        i32 ColorDiffuseLoc = GetShaderLocation(app_state->custom_shader, "colorDiffuse");
        f64 DiffuseValue[4] = {1.0, 1.0, 1.0, 1.0};
        SetShaderValue(app_state->custom_shader, ColorDiffuseLoc, &DiffuseValue, SHADER_UNIFORM_VEC4);

        // Like the sun shining on the earth
        CreateLight(LIGHT_DIRECTIONAL, (Vector3){1000.0f, 1000.0f, 0.0f}, Vector3Zero(), WHITE, app_state->custom_shader);

        // @Note(Victor): We can add more lights to the scene to better show the colors of the galaxies
        CreateLight(LIGHT_DIRECTIONAL, (Vector3){-1000.0f, -1000.0f, 0.0f}, Vector3Zero(), WHITE, app_state->custom_shader);
        CreateLight(LIGHT_DIRECTIONAL, (Vector3){0.0f, 0.0f, 1000.0f}, Vector3Zero(), WHITE, app_state->custom_shader);
        CreateLight(LIGHT_DIRECTIONAL, (Vector3){0.0f, 0.0f, -1000.0f}, Vector3Zero(), WHITE, app_state->custom_shader);

        // We can also add a point light at the center of the earth
        CreateLight(LIGHT_POINT, (Vector3){0.0f, 0.0f, 0.0f}, Vector3Zero(), WHITE, app_state->custom_shader);
    }

    // Material
    {
        // NOTE: We are assigning the intancing shader to material.shader
        // to be used on mesh drawing with DrawMeshInstanced()
        Material GalaxyMaterial = LoadMaterialDefault();
        GalaxyMaterial.shader = app_state->custom_shader;

        GalaxyMaterial.maps[MATERIAL_MAP_DIFFUSE].texture = LoadTexture("./resources/images/galaxy_test_texture_diffuse.png");
        GalaxyMaterial.maps[MATERIAL_MAP_SPECULAR].texture = LoadTexture("./resources/images/galaxy_test_texture_specular.png");

        GalaxyMaterial.maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
        GalaxyMaterial.maps[MATERIAL_MAP_SPECULAR].value = 1.0f;

        // Set the shininess for specular reflections
        float shininess = 32.0f;
        SetShaderValue(GalaxyMaterial.shader, GetShaderLocation(GalaxyMaterial.shader, "shininess"), &shininess, SHADER_UNIFORM_FLOAT);

        app_state->material_instance = GalaxyMaterial;
        app_state->material_instance.shader = app_state->custom_shader;
        app_state->material_instance.maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
    }

    printf("\n\tMemory usage before we start the game loop\n");
    print_memory_usage(app_state);

    // Load the Earth model
    app_state->earth_model = LoadModel("./resources/Earth_1_12756.glb");

    // Optionally, you can scale the model if needed
    Matrix scaleMatrix = MatrixScale(0.01f, 0.01f, 0.01f);
    app_state->earth_model.transform = MatrixMultiply(app_state->earth_model.transform, scaleMatrix);

    // Main loop
    while (!WindowShouldClose()) // Detect window close button or ESC key
    {
        f64 DeltaTime = GetFrameTime();
        game_update(app_state, DeltaTime);
        game_render(app_state, DeltaTime);
    }

    cleanup(app_state);

    return (0);
}
