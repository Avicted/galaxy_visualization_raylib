// Defines -----------------------------------------------------------------------
#define RLIGHTS_IMPLEMENTATION

// Includes ----------------------------------------------------------------------
#include "includes.h"
#include "raylib_includes.h"

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

    DRAW_DATA_COUNT
} draw_data_t;

typedef struct
{
    u64 cpu_memory_allocated;

    arcmin_data_t arcmin_data;
    draw_data_t draw_data;

    // @Note(Victor): Data from the course, only celestial coordinates, no redshift (distance)
    arcmin_data_t *data_points_a;
    arcmin_data_t *data_points_b;

    bool debug;
    bool data_is_loaded;
    bool is_paused;
    bool cursor_enabled;

    Font main_font;

    i32 window_width;
    i32 window_height;

    Camera3D main_camera;
    f64 camera_zoom;
    f64 camera_yaw;
    f64 camera_pitch;
    Vector3 camera_direction;

    Shader custom_shader;

    Mesh sphere_mesh;
    Model earth_model;

    // Batch rendering in Raylib with a custom shader
    Material material_instance;
    Matrix *matrix_transforms_a;
    Matrix *matrix_transforms_b;

    draw_data_t data_to_draw;

} app_state_t;

// Constants ---------------------------------------------------------------------
const ul MAX_DATA_POINTS = 100000UL;

const char *data_a_filename = "./input_data/data_100k_arcmin.txt";
const char *data_b_filename = "./input_data/flat_100k_arcmin.txt";

// Forward declarations ----------------------------------------------------------
internal i32 app_platform_init(app_state_t *app_state);
internal i32 app_init_shaders(app_state_t *app_state);
internal i32 app_read_input_data(app_state_t *app_state);
internal i32 app_init(app_state_t *app_state);
internal void app_render(app_state_t *app_state, f64 dt);
internal void app_update(app_state_t *app_state, f64 dt);
internal void app_cleanup(app_state_t *app_state);

internal i32 upload_matrix_transforms_to_gpu(app_state_t *app_state);
internal i32 initialize_transforms_course_data(app_state_t *app_state);
internal bool read_input_data_from_file(const char *FileName, arcmin_data_t *DataPointsLocation);
internal void print_memory_usage(app_state_t *app_state);
internal void rotate_camera_around_origo(app_state_t *app_state, f64 dt);
internal void handle_window_resize(app_state_t *app_state);
internal void parse_input_args(app_state_t *app_state, i32 argc, char **argv);

i32 main(i32 argc, char **argv)
{
    app_state_t *app_state = (app_state_t *)calloc(1, sizeof(app_state_t));
    if (app_state == NULL)
    {
        printf("Error allocating memory for app_state!\n");
        return (1);
    }

    parse_input_args(app_state, argc, argv);

    i32 app_init_result = app_init(app_state);
    if (app_init_result != 0)
    {
        fprintf(stderr, "ERROR: app_init failed.\n");
        return (1);
    }

    // Main loop
    while (!WindowShouldClose())
    {
        f64 dt = GetFrameTime();
        app_update(app_state, dt);
        app_render(app_state, dt);
    }

    app_cleanup(app_state);

    return (0);
}

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

    if ((IsKeyPressed(KEY_ENTER) && (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT))) ||
        (IsKeyPressed(KEY_F11)))
    {
        i32 current_display = GetCurrentMonitor();

        if (IsWindowFullscreen())
        {
            // If we are full screen, then go back to the windowed size
            SetWindowSize(app_state->window_width, app_state->window_height);
        }
        else
        {
            // If we are not full screen, set the window size to match the monitor we are on
            SetWindowSize(GetMonitorWidth(current_display), GetMonitorHeight(current_display));
        }

        ToggleFullscreen();

        app_state->window_width = GetScreenWidth();
        app_state->window_height = GetScreenHeight();
    }
}

internal void
rotate_camera_around_origo(app_state_t *app_state, f64 dt)
{
    Camera3D *cam = &app_state->main_camera;
    f64 speed = 10.0f * dt;
    f64 vertical_speed = 5.0f * dt;

    f64 *yaw = &app_state->camera_yaw;
    f64 *pitch = &app_state->camera_pitch;
    Vector3 *direction = &app_state->camera_direction;

    local_persist bool prev_is_paused = false;

    // Detect transition into free look mode
    bool entered_free_look = (app_state->is_paused && !prev_is_paused);

    if (app_state->is_paused)
    {
        if (entered_free_look)
        {
            HideCursor();
            app_state->cursor_enabled = false;

            // Set the camera to look at the data from the earth's position
            cam->position = (Vector3){13.632f, 1.377f, 9.318f};
            cam->target = (Vector3){14.176f, 1.954f, 9.927f};
            *direction = (Vector3){0.545f, 0.577f, 0.609f};
            *yaw = 48.0f;
            *pitch = 35.220f;
        }

        const Vector2 mouse_delta = GetMouseDelta();
        *yaw += mouse_delta.x * 0.1f;
        *pitch -= mouse_delta.y * 0.1f;

        const Vector2 mouse_pos = {GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};
        SetMousePosition(mouse_pos.x, mouse_pos.y);

        // Clamp pitch to avoid flipping the camera
        if (*pitch > 89.0f)
        {
            *pitch = 89.0f;
        }
        if (*pitch < -89.0f)
        {
            *pitch = -89.0f;
        }

        direction->x = cosf(DEG2RAD * (*pitch)) * cosf(DEG2RAD * (*yaw));
        direction->y = sinf(DEG2RAD * (*pitch));
        direction->z = cosf(DEG2RAD * (*pitch)) * sinf(DEG2RAD * (*yaw));
        *direction = Vector3Normalize(*direction);

        const Vector3 right = Vector3Normalize(Vector3CrossProduct(*direction, cam->up));
        const Vector3 up = Vector3Normalize(Vector3CrossProduct(right, *direction));

        if (IsKeyDown(KEY_LEFT_SHIFT))
        {
            speed *= 0.1f;
            vertical_speed *= 0.1f;
        }

        if (IsKeyDown(KEY_W))
        {
            cam->position = Vector3Add(cam->position, Vector3Scale(*direction, speed));
        }
        if (IsKeyDown(KEY_S))
        {
            cam->position = Vector3Subtract(cam->position, Vector3Scale(*direction, speed));
        }
        if (IsKeyDown(KEY_D))
        {
            cam->position = Vector3Add(cam->position, Vector3Scale(right, speed));
        }
        if (IsKeyDown(KEY_A))
        {
            cam->position = Vector3Subtract(cam->position, Vector3Scale(right, speed));
        }
        if (IsKeyDown(KEY_Q))
        {
            cam->position = Vector3Subtract(cam->position, Vector3Scale(up, vertical_speed));
        }
        if (IsKeyDown(KEY_E))
        {
            cam->position = Vector3Add(cam->position, Vector3Scale(up, vertical_speed));
        }

        cam->target = Vector3Add(cam->position, *direction);
    }
    else
    {
        if (prev_is_paused)
        {
            ShowCursor();
            app_state->cursor_enabled = true;
        }

        local_persist f64 previous_time_since_start = 0.0f;
        previous_time_since_start += dt * 0.2f;

        cam->position.x = 25.0f * cosf(previous_time_since_start) * app_state->camera_zoom;
        cam->position.y = 50.0f;
        cam->position.z = 25.0f * sinf(previous_time_since_start) * app_state->camera_zoom;

        cam->target = Vector3Zero();
    }

    prev_is_paused = app_state->is_paused;
}

internal void
app_update(app_state_t *app_state, f64 dt)
{
    handle_window_resize(app_state);

    if (IsKeyPressed(KEY_ESCAPE))
    {
        CloseWindow();
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

    if (IsKeyPressed(KEY_SPACE))
    {
        app_state->is_paused = !app_state->is_paused;
        printf("\tis_paused: %s\n", app_state->is_paused ? "true" : "false");
    }

    rotate_camera_around_origo(app_state, dt);

    f64 scroll = GetMouseWheelMove();
    if (scroll != 0.0f)
    {
        const f64 zoom_change = -2.5f;
        f64 Speed = zoom_change;
        app_state->camera_zoom = Clamp(app_state->camera_zoom + scroll * Speed * dt, 0.0f, 10.0f);
    }
}

internal void
app_render(app_state_t *app_state, f64 dt)
{
    (void)dt;

    BeginDrawing();
    ClearBackground(BLACK);

    if (!app_state->data_is_loaded)
    {
        return;
    }

    BeginMode3D(app_state->main_camera);

    DrawSphere((Vector3){0.0f, 0.0f, 0.0f}, 1.0f, BLUE);

    Vector3 earth_pos = {0.0f, 0.0f, 0.0f};
    const f64 earth_scale = 1.0f;
    DrawModel(app_state->earth_model, earth_pos, earth_scale, WHITE);

    if (app_state->data_to_draw == DRAW_DATA_A || app_state->data_to_draw == DRAW_DATA_ALL)
    {
        const Color DARK_BLUE = {0, 0, 255, 255};
        app_state->material_instance.maps[MATERIAL_MAP_DIFFUSE].color = DARK_BLUE;
        DrawMeshInstanced(app_state->sphere_mesh, app_state->material_instance, app_state->matrix_transforms_a, MAX_DATA_POINTS);
    }

    if (app_state->data_to_draw == DRAW_DATA_B || app_state->data_to_draw == DRAW_DATA_ALL)
    {
        app_state->material_instance.maps[MATERIAL_MAP_DIFFUSE].color = RED;
        DrawMeshInstanced(app_state->sphere_mesh, app_state->material_instance, app_state->matrix_transforms_b, MAX_DATA_POINTS);
    }

    EndMode3D();

    // UI -----------------------------------------------------------------------------
    u8 font_size = 30;
    u8 font_spacing = 2;

    DrawTextEx(
        app_state->main_font,
        TextFormat("FPS: %i", GetFPS()), (Vector2){10, 10},
        font_size,
        font_spacing,
        WHITE);

    font_size = 24;
    font_spacing = 2;

    if (!app_state->is_paused)
    {
        DrawTextEx(
            app_state->main_font,
            TextFormat("Scroll to zoom: %.2f", app_state->camera_zoom),
            (Vector2){10, 50},
            font_size,
            font_spacing,
            WHITE);
    }

    DrawTextEx(
        app_state->main_font,
        TextFormat("Press F11 to toggle fullscreen"),
        (Vector2){10, 70},
        font_size,
        font_spacing,
        WHITE);

    DrawTextEx(
        app_state->main_font,
        TextFormat("Press 1, 2, or 3 to toggle which data to draw"),
        (Vector2){10, 90},
        font_size,
        font_spacing,
        WHITE);

    DrawTextEx(
        app_state->main_font,
        TextFormat("Red are uniformly distributed"),
        (Vector2){10, 110},
        font_size,
        font_spacing,
        (Color){255, 128, 0, 255});

    DrawTextEx(
        app_state->main_font,
        TextFormat("Blue are real data"),
        (Vector2){10, 130},
        font_size,
        font_spacing,
        (Color){0, 128, 255, 255});

    if (app_state->is_paused)
    {
        DrawTextEx(
            app_state->main_font,
            TextFormat("Press W, A, S, D, Q, E to move the camera + Mouse"),
            (Vector2){10, 150},
            font_size,
            font_spacing,
            WHITE);

        DrawTextEx(
            app_state->main_font,
            TextFormat("Press LShift to move slower"),
            (Vector2){10, 170},
            font_size,
            font_spacing,
            WHITE);
    }

    font_size = 30;
    if (app_state->is_paused)
    {
        const f64 text_width = MeasureText("Press Space again to go back to Auto Look", font_size);
        DrawTextEx(
            app_state->main_font,
            TextFormat("Press Space again to go back to Auto Look"),
            (Vector2){(f32)(app_state->window_width / 2.0f - (f32)text_width + 500.0f / 2.0f), (f32)app_state->window_height - 100.0f},
            font_size,
            font_spacing,
            PURPLE);
    }
    else
    {
        const f64 text_width = MeasureText("Press Space to enter Free Look mode", font_size);
        DrawTextEx(
            app_state->main_font,
            "Press Space to enter Free Look mode",
            (Vector2){(f32)(app_state->window_width / 2.0f - (f32)text_width + 500.0f / 2.0f), (f32)app_state->window_height - 100.0f},
            font_size,
            font_spacing,
            GREEN);
    }

    font_size = 30;
    if (app_state->is_paused)
    {
        const char *is_paused_text = "Free Look";
        DrawTextEx(
            app_state->main_font,
            is_paused_text,
            (Vector2){app_state->window_width - 128.0f - 64.0f, 30},
            font_size,
            font_spacing,
            PURPLE);
    }
    else
    {
        const char *is_paused_text = "Auto Look";
        DrawTextEx(
            app_state->main_font,
            is_paused_text,
            (Vector2){app_state->window_width - 64.0f - 128.0f - 32.0f, 30},
            font_size,
            font_spacing,
            GREEN);
    }

    EndDrawing();
}

internal void
print_memory_usage(app_state_t *app_state)
{
    printf("\n\tMemory used in GB: %f\n", (f64)app_state->cpu_memory_allocated / (f64)Gigabytes(1));
    printf("\tMemory used in MB: %f\n", (f64)app_state->cpu_memory_allocated / (f64)Megabytes(1));
}

internal void
app_cleanup(app_state_t *app_state)
{
    CloseWindow();
    printf("\n\tClosed window and OpenGL context\n");

    free(app_state->data_points_a);
    app_state->cpu_memory_allocated -= MAX_DATA_POINTS * sizeof(arcmin_data_t);
    printf("\n\tFreeing data_points_a %lu\n", MAX_DATA_POINTS * sizeof(arcmin_data_t));
    print_memory_usage(app_state);

    free(app_state->data_points_b);
    app_state->cpu_memory_allocated -= MAX_DATA_POINTS * sizeof(arcmin_data_t);
    printf("\n\tFreeing data_points_b: %lu\n", MAX_DATA_POINTS * sizeof(arcmin_data_t));
    print_memory_usage(app_state);

    free(app_state->matrix_transforms_a);
    app_state->cpu_memory_allocated -= MAX_DATA_POINTS * sizeof(Matrix);
    printf("\n\tFreeing matrix_transforms_a: %lu\n", MAX_DATA_POINTS * sizeof(Matrix));
    print_memory_usage(app_state);

    free(app_state->matrix_transforms_b);
    app_state->cpu_memory_allocated -= MAX_DATA_POINTS * sizeof(Matrix);
    printf("\n\tFreeing matrix_transforms_b: %lu\n", MAX_DATA_POINTS * sizeof(Matrix));
    print_memory_usage(app_state);

    // @Note(Victor): There should be no allocated memory left
    Assert(app_state->cpu_memory_allocated == 0);
}

internal bool
read_input_data_from_file(const char *file_name, arcmin_data_t *data_points_location)
{
    FILE *f = fopen(file_name, "r");
    if (f == NULL)
    {
        printf("Error opening file!\n");
        return (false);
    }

    // Read the header
    char line[1024];
    if (fgets(line, sizeof(line), f) == NULL)
    {
        printf("Error reading header!\n");
        return (false);
    }

    // Read the data into the DataPointsLocation the data is in arcmin declination and right ascension \t separated
    i32 i = 0;
    while (fgets(line, sizeof(line), f) != NULL)
    {
        // @Note(Victor): We expect the input data to be separated by tabs !!!
        // Parse the line
        char *token = strtok(line, "\t");
        i32 j = 0;
        while (token != NULL)
        {
            if (j == 0)
            {
                data_points_location[i].right_ascension = atof(token);
            }
            else if (j == 1)
            {
                data_points_location[i].declination = atof(token);
            }
            else
            {
                printf("Error parsing line!\n");
                return (false);
            }

            token = strtok(NULL, "\t");
            j++;
        }

        i++;
    }

    fclose(f);

    return (true);
}

internal i32
app_init(app_state_t *app_state)
{
    app_state->cpu_memory_allocated = 0L;
    app_state->debug = false;
    app_state->data_is_loaded = false;
    app_state->is_paused = false;
    app_state->cursor_enabled = true;

    app_state->data_points_a = NULL;
    app_state->data_points_b = NULL;

    app_state->main_camera = (Camera3D){0};
    app_state->main_camera.position = (Vector3){0.0f, 0.0f, 0.0f};
    app_state->main_camera.target = (Vector3){0.0f, 0.0f, 0.0f};
    app_state->main_camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    app_state->main_camera.fovy = 65.0f;
    app_state->main_camera.projection = CAMERA_PERSPECTIVE;

    app_state->camera_zoom = 1.0f * PI;
    app_state->camera_yaw = 45.80f;
    app_state->camera_pitch = 42.12f;
    app_state->camera_direction = (Vector3){0};

    app_state->custom_shader = (Shader){0};
    app_state->material_instance = (Material){0};
    app_state->sphere_mesh = (Mesh){0};
    app_state->earth_model = (Model){0};

    app_state->matrix_transforms_a = NULL;
    app_state->matrix_transforms_b = NULL;

    app_state->data_to_draw = DRAW_DATA_ALL;

    i32 app_read_input_data_result = app_read_input_data(app_state);
    if (app_read_input_data_result != 0)
    {
        fprintf(stderr, "ERROR: Could not perform app_read_input_data.\n");
        return (1);
    }

    printf("\tHello from raylib_galaxy_application!\n\n");

    i32 matrix_gpu_upload_result = upload_matrix_transforms_to_gpu(app_state);
    if (matrix_gpu_upload_result != 0)
    {
        fprintf(stderr, "ERROR: Could not upload matrix transforms to the GPU.\n");
        return (1);
    }

    i32 platform_init_result = app_platform_init(app_state);
    if (platform_init_result != 0)
    {
        fprintf(stderr, "ERROR: Could not initialize the platform API (Raylib)\n");
        return (1);
    }

    app_state->main_font = LoadFontEx("./resources/fonts/retro-pixel-arcade.ttf", 128, 0, 250);

    app_state->sphere_mesh = GenMeshSphere(0.2f, 4, 4);

    i32 shader_init_result = app_init_shaders(app_state);
    if (shader_init_result != 0)
    {
        fprintf(stderr, "ERROR: Could not initialize the shader(s)\n");
        return (1);
    }

    printf("\n\tMemory usage before we start the main program loop\n");
    print_memory_usage(app_state);

    app_state->earth_model = LoadModel("./resources/Earth_1_12756.glb");
    Matrix earch_scale_matrix = MatrixScale(0.01f, 0.01f, 0.01f);
    app_state->earth_model.transform = MatrixMultiply(app_state->earth_model.transform, earch_scale_matrix);

    return (0);
}

internal i32
app_read_input_data(app_state_t *app_state)
{
    app_state->data_points_a = (arcmin_data_t *)calloc(MAX_DATA_POINTS, sizeof(arcmin_data_t));
    if (app_state->data_points_a == NULL)
    {
        printf("Error allocating memory for data_points_a!\n");
        free(app_state);
        return (1);
    }
    app_state->cpu_memory_allocated += MAX_DATA_POINTS * sizeof(arcmin_data_t);

    app_state->data_points_b = (arcmin_data_t *)calloc(MAX_DATA_POINTS, sizeof(arcmin_data_t));
    if (app_state->data_points_b == NULL)
    {
        printf("Error allocating memory for data_points_b!\n");
        free(app_state->data_points_a);
        free(app_state);
        return (1);
    }
    app_state->cpu_memory_allocated += MAX_DATA_POINTS * sizeof(arcmin_data_t);

    if (read_input_data_from_file(data_a_filename, app_state->data_points_a))
    {
        printf("\tread_input_data_from_file: %s succeeded!\n", data_a_filename);
    }
    else
    {
        printf("\tread_input_data_from_file: %s failed!\n", data_a_filename);
        app_cleanup(app_state);
        return (1);
    }

    if (read_input_data_from_file(data_b_filename, app_state->data_points_b))
    {
        printf("\tread_input_data_from_file: %s succeeded!\n", data_b_filename);
    }
    else
    {
        printf("\tread_input_data_from_file: %s failed!\n", data_b_filename);
        app_cleanup(app_state);
        return (1);
    }

    // Assert that all the input data was read ----------------------------------------
    ul data_count_read = 0;
    for (ul i = 0; i < MAX_DATA_POINTS; ++i)
    {
        if (app_state->data_points_a[i].right_ascension != 0.0f)
        {
            data_count_read++;
        }
    }

    Assert(data_count_read == MAX_DATA_POINTS);
    Assert(app_state->data_points_b != NULL);

    data_count_read = 0;
    for (ul i = 0; i < MAX_DATA_POINTS; ++i)
    {
        if (app_state->data_points_b[i].right_ascension != 0.0f)
        {
            data_count_read++;
        }
    }

    Assert(data_count_read == MAX_DATA_POINTS);

    app_state->data_is_loaded = true;

    return (0);
}

internal i32
initialize_transforms_course_data(app_state_t *app_state)
{
    for (ul i = 0; i < MAX_DATA_POINTS; ++i)
    {
        // data_points_a real galaxies
        {
            // Transform the arc minutes into radians that the trigonometric functions take as input. (sinf, cosf, tanf)
            const f64 right_ascension_rad = (app_state->data_points_a[i].right_ascension / 60.0f) * DEG2RAD;
            const f64 declination_rad = (app_state->data_points_a[i].declination / 60.0f) * DEG2RAD;

            // Calculate the position on the sphere using spherical coordinates
            const f64 radius = 50.0f;
            const f64 x = radius * cosf(right_ascension_rad) * cosf(declination_rad);
            const f64 y = radius * sinf(declination_rad);
            const f64 z = radius * sinf(right_ascension_rad) * cosf(declination_rad);

            // Create a model matrix for each data point to position it
            app_state->matrix_transforms_a[i] = MatrixIdentity();
            app_state->matrix_transforms_a[i] = MatrixMultiply(app_state->matrix_transforms_a[i], MatrixScale(0.1f, 0.1f, 0.1f));
            app_state->matrix_transforms_a[i] = MatrixMultiply(app_state->matrix_transforms_a[i], MatrixTranslate(x, y, z));
        }

        // data_points_b uniformly distributed (galaxies)
        {
            const f64 right_ascension_rad = (app_state->data_points_b[i].right_ascension / 60.0f) * DEG2RAD;
            const f64 declination_rad = (app_state->data_points_b[i].declination / 60.0f) * DEG2RAD;

            // Calculate the position on the sphere using spherical coordinates
            const f64 radius = 50.0f;
            const f64 x = radius * cosf(right_ascension_rad) * cosf(declination_rad);
            const f64 y = radius * sinf(declination_rad);
            const f64 z = radius * sinf(right_ascension_rad) * cosf(declination_rad);

            // Create a model matrix for each data point to position it
            app_state->matrix_transforms_b[i] = MatrixIdentity();
            app_state->matrix_transforms_b[i] = MatrixMultiply(app_state->matrix_transforms_b[i], MatrixScale(0.1f, 0.1f, 0.1f));
            app_state->matrix_transforms_b[i] = MatrixMultiply(app_state->matrix_transforms_b[i], MatrixTranslate(x, y, z));
        }
    }

    return (0);
}

internal i32
upload_matrix_transforms_to_gpu(app_state_t *app_state)
{

    app_state->matrix_transforms_a = (Matrix *)calloc(MAX_DATA_POINTS, sizeof(Matrix));
    if (app_state->matrix_transforms_a == NULL)
    {
        fprintf(stderr, "ERROR: Could not allocate matrix_transforms_a.\n");
        return (1);
    }
    app_state->cpu_memory_allocated += MAX_DATA_POINTS * sizeof(Matrix);

    app_state->matrix_transforms_b = (Matrix *)calloc(MAX_DATA_POINTS, sizeof(Matrix));
    if (app_state->matrix_transforms_b == NULL)
    {
        fprintf(stderr, "ERROR: Could not allocate matrix_transforms_b.\n");
        return (1);
    }
    app_state->cpu_memory_allocated += MAX_DATA_POINTS * sizeof(Matrix);

    i32 course_data_init_result = initialize_transforms_course_data(app_state);
    if (course_data_init_result != 0)
    {
        fprintf(stderr, "ERROR: Could not initialize course data matrix transforms.\n");
        return (1);
    }

    return (0);
}

internal i32
app_init_shaders(app_state_t *app_state)
{
    app_state->custom_shader = LoadShader("./shaders/lighting_instancing.vs", "./shaders/lighting.fs");
    app_state->custom_shader.locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(app_state->custom_shader, "mvp");
    app_state->custom_shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(app_state->custom_shader, "viewPos");
    app_state->custom_shader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocationAttrib(app_state->custom_shader, "instanceTransform");

    // Lighting
    {
        // Setting shader values
        i32 ambient_loc = GetShaderLocation(app_state->custom_shader, "ambient");
        f64 ambient_value[4] = {1.0, 1.0, 1.0, 1.0};
        SetShaderValue(app_state->custom_shader, ambient_loc, &ambient_value, SHADER_UNIFORM_VEC4);

        i32 color_diffuse_loc = GetShaderLocation(app_state->custom_shader, "colorDiffuse");
        f64 diffuse_value[4] = {1.0, 1.0, 1.0, 1.0};
        SetShaderValue(app_state->custom_shader, color_diffuse_loc, &diffuse_value, SHADER_UNIFORM_VEC4);

        // The sun shining on the earth
        CreateLight(LIGHT_DIRECTIONAL, (Vector3){1000.0f, 1000.0f, 0.0f}, Vector3Zero(), WHITE, app_state->custom_shader);

        // @Note(Victor): We add more lights to the scene to better show the colors of the galaxies
        CreateLight(LIGHT_DIRECTIONAL, (Vector3){-1000.0f, -1000.0f, 0.0f}, Vector3Zero(), WHITE, app_state->custom_shader);
        CreateLight(LIGHT_DIRECTIONAL, (Vector3){0.0f, 0.0f, 1000.0f}, Vector3Zero(), WHITE, app_state->custom_shader);
        CreateLight(LIGHT_DIRECTIONAL, (Vector3){0.0f, 0.0f, -1000.0f}, Vector3Zero(), WHITE, app_state->custom_shader);

        // We also add a point light at the center of the earth
        CreateLight(LIGHT_POINT, (Vector3){0.0f, 0.0f, 0.0f}, Vector3Zero(), WHITE, app_state->custom_shader);
    }

    // Material
    {
        Material galaxy_material = LoadMaterialDefault();
        galaxy_material.shader = app_state->custom_shader;

        galaxy_material.maps[MATERIAL_MAP_DIFFUSE].texture = LoadTexture("./resources/images/galaxy_test_texture_diffuse.png");
        galaxy_material.maps[MATERIAL_MAP_SPECULAR].texture = LoadTexture("./resources/images/galaxy_test_texture_specular.png");

        galaxy_material.maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
        galaxy_material.maps[MATERIAL_MAP_SPECULAR].value = 1.0f;

        f32 shininess = 32.0f;
        SetShaderValue(galaxy_material.shader, GetShaderLocation(galaxy_material.shader, "shininess"), &shininess, SHADER_UNIFORM_FLOAT);

        app_state->material_instance = galaxy_material;
        app_state->material_instance.shader = app_state->custom_shader;
        app_state->material_instance.maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
    }

    return (0);
}

internal i32
app_platform_init(app_state_t *app_state)
{
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);

    InitWindow(app_state->window_width, app_state->window_height, "galaxy_visuazation_raylib");

    SetTargetFPS(60);
    SetWindowIcon(LoadImage("./resources/images/app_icon.png"));

    return (0);
}
