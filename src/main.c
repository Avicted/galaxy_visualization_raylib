// Defines -----------------------------------------------------------------------
#define RLIGHTS_IMPLEMENTATION
#define INITIAL_WINDOW_WIDTH 1280
#define INITIAL_WINDOW_HEIGHT 720

// Includes ----------------------------------------------------------------------
#include "redefines.h"
#include "includes.h"
#include "macros.h"
#include "raylib_includes.h"

// Types -------------------------------------------------------------------------
typedef struct
{
    f64 right_ascension;
    f64 declination;
} arcmin_data_t;

typedef enum
{
    DRAW_DATA_A,
    DRAW_DATA_B,
    DRAW_DATA_ALL,
    DRAW_DATA_COUNT,
} draw_data_t;

typedef struct
{
    // @Note(Victor): Data from the ÅA course, only celestial coordinates, no redshift (distance)
    arcmin_data_t *data_points_a;
    arcmin_data_t *data_points_b;
    ul data_point_count;

    draw_data_t data_to_draw;

    u64 cpu_memory_allocated;
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

    // Batch rendering
    Material material_instance;
    Matrix *matrix_transforms_a;
    Matrix *matrix_transforms_b;
} app_state_t;

// Constants ---------------------------------------------------------------------
global_variable const ul MAX_DATA_POINTS = 100000UL;
global_variable const char *data_a_filename = "./input_data/data_100k_arcmin.txt";
global_variable const char *data_b_filename = "./input_data/flat_100k_arcmin.txt";

// Forward declarations ----------------------------------------------------------
internal i32 app_init(app_state_t *app_state);
internal i32 app_init_platform(app_state_t *app_state);
internal i32 app_init_shaders(app_state_t *app_state);
internal i32 app_read_input_data(app_state_t *app_state);
internal void app_run(app_state_t *app_state);
internal void app_update(app_state_t *app_state, f64 dt);
internal void app_render(app_state_t *app_state, f64 dt);
internal void app_cleanup(app_state_t *app_state);

internal usize read_input_data_from_file(const char *file_name, arcmin_data_t *data_points_location, ul max_points);
internal i32 upload_matrix_transforms_to_gpu(app_state_t *app_state);
internal i32 initialize_transforms_course_data(app_state_t *app_state);
internal void parse_input_args(app_state_t *app_state, i32 argc, char **argv);
internal inline void handle_window_resize(app_state_t *app_state);
internal inline void rotate_camera_around_origo(app_state_t *app_state, f64 dt);
internal void print_memory_usage(app_state_t *app_state);

i32 main(i32 argc, char **argv)
{
    app_state_t *app_state = (app_state_t *)calloc(1, sizeof(app_state_t));
    if (app_state == NULL)
    {
        printf("Error allocating memory for app_state!\n");
        return 1;
    }

    parse_input_args(app_state, argc, argv);

    i32 app_init_result = app_init(app_state);
    if (app_init_result != 0)
    {
        fprintf(stderr, "ERROR: app_init failed.\n");
        return 1;
    }

    app_run(app_state);

    app_cleanup(app_state);

    return 0;
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

internal inline void
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

internal inline void
rotate_camera_around_origo(app_state_t *app_state, f64 dt)
{
    Camera3D *cam = &app_state->main_camera;
    f64 speed = 10.0f * dt;
    f64 vertical_speed = 5.0f * dt;

    f64 *yaw = &app_state->camera_yaw;
    f64 *pitch = &app_state->camera_pitch;
    Vector3 *direction = &app_state->camera_direction;

    // Detect transition into free look mode
    local_persist bool prev_is_paused = false;
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

        if (IsKeyDown(KEY_LEFT_CONTROL))
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
        if (IsKeyDown(KEY_LEFT_SHIFT))
        {
            cam->position = Vector3Subtract(cam->position, Vector3Scale(up, vertical_speed));
        }
        if (IsKeyDown(KEY_SPACE))
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
app_run(app_state_t *app_state)
{
    while (!WindowShouldClose())
    {
        const f64 dt = GetFrameTime();
        app_update(app_state, dt);
        app_render(app_state, dt);
    }
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

    if (IsKeyPressed(KEY_R))
    {
        app_state->is_paused = !app_state->is_paused;
        printf("\tis_paused: %s\n", app_state->is_paused ? "true" : "false");
    }

    rotate_camera_around_origo(app_state, dt);

    f64 scroll = GetMouseWheelMove();
    if (scroll != 0.0f)
    {
        const f64 zoom_change = -32.0f;
        app_state->camera_zoom = Clamp(app_state->camera_zoom + scroll * zoom_change * dt, 0.5f, 5.0f);
    }
}

internal void
app_render(app_state_t *app_state, f64 dt)
{
    (void)dt;

    if (!app_state->data_is_loaded)
    {
        return;
    }

    BeginDrawing();
    ClearBackground(BLACK);

    BeginMode3D(app_state->main_camera);

    Vector3 earth_pos = {0.0f, 0.0f, 0.0f};
    const f64 earth_scale = 1.0f;
    DrawModel(app_state->earth_model, earth_pos, earth_scale, WHITE);

    if (app_state->data_to_draw == DRAW_DATA_A || app_state->data_to_draw == DRAW_DATA_ALL)
    {
        const Color DARK_BLUE = {0, 0, 255, 255};
        app_state->material_instance.maps[MATERIAL_MAP_DIFFUSE].color = DARK_BLUE;

        DrawMeshInstanced(
            app_state->sphere_mesh,
            app_state->material_instance,
            app_state->matrix_transforms_a,
            app_state->data_point_count);
    }

    if (app_state->data_to_draw == DRAW_DATA_B || app_state->data_to_draw == DRAW_DATA_ALL)
    {
        app_state->material_instance.maps[MATERIAL_MAP_DIFFUSE].color = RED;

        DrawMeshInstanced(
            app_state->sphere_mesh,
            app_state->material_instance,
            app_state->matrix_transforms_b,
            app_state->data_point_count);
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
        TextFormat("Press F11 (ALT + ENTER) to toggle fullscreen"),
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
            TextFormat("Press WASD (Shift, Space) to move + mouse look"),
            (Vector2){10, 150},
            font_size,
            font_spacing,
            WHITE);

        DrawTextEx(
            app_state->main_font,
            TextFormat("Press LControl to move slower"),
            (Vector2){10, 170},
            font_size,
            font_spacing,
            WHITE);
    }

    font_size = 30;
    if (app_state->is_paused)
    {
        const f64 text_width = MeasureText("Press R again to go back to Auto Look", font_size);
        DrawTextEx(
            app_state->main_font,
            TextFormat("Press R again to go back to Auto Look"),
            (Vector2){(f32)(app_state->window_width / 2.0f - (f32)text_width + 500.0f / 2.0f), (f32)app_state->window_height - 100.0f},
            font_size,
            font_spacing,
            PURPLE);
    }
    else
    {
        const f64 text_width = MeasureText("Press R to enter Free Look mode", font_size);
        DrawTextEx(
            app_state->main_font,
            "Press R to enter Free Look mode",
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
            (Vector2){app_state->window_width - 128.0f - 95.0f, 30},
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
    if (!app_state)
    {
        return;
    }

    if (app_state->earth_model.meshCount > 0)
    {
        UnloadModel(app_state->earth_model);
    }
    if (app_state->sphere_mesh.vboId)
    {
        UnloadMesh(app_state->sphere_mesh);
    }
    if (app_state->material_instance.shader.id)
    {
        UnloadShader(app_state->material_instance.shader);
    }

    if (app_state->material_instance.maps[MATERIAL_MAP_DIFFUSE].texture.id)
    {
        UnloadTexture(app_state->material_instance.maps[MATERIAL_MAP_DIFFUSE].texture);
    }
    if (app_state->material_instance.maps[MATERIAL_MAP_SPECULAR].texture.id)
    {
        UnloadTexture(app_state->material_instance.maps[MATERIAL_MAP_SPECULAR].texture);
    }

    if (app_state->main_font.texture.id)
    {
        UnloadFont(app_state->main_font);
    }

    if (app_state->data_points_a)
    {
        free(app_state->data_points_a);
        app_state->cpu_memory_allocated -= (usize)MAX_DATA_POINTS * sizeof(arcmin_data_t);
    }
    if (app_state->data_points_b)
    {
        free(app_state->data_points_b);
        app_state->cpu_memory_allocated -= (usize)MAX_DATA_POINTS * sizeof(arcmin_data_t);
    }
    if (app_state->matrix_transforms_a)
    {
        free(app_state->matrix_transforms_a);
        app_state->cpu_memory_allocated -= (usize)MAX_DATA_POINTS * sizeof(Matrix);
    }
    if (app_state->matrix_transforms_b)
    {
        free(app_state->matrix_transforms_b);
        app_state->cpu_memory_allocated -= (usize)MAX_DATA_POINTS * sizeof(Matrix);
    }

    printf("\nMemory usage at the end of the program:");
    print_memory_usage(app_state);

    ASSERT(app_state->cpu_memory_allocated == 0);

    CloseWindow();

    free(app_state);
}

internal usize
read_input_data_from_file(const char *file_name, arcmin_data_t *data_points_location, ul max_points)
{
    FILE *file = fopen(file_name, "r");
    if (file == NULL)
    {
        fprintf(stderr, "Error opening file: %s\n", file_name);
        return -1;
    }

    char line[1024];

    if (fgets(line, sizeof(line), file) == NULL)
    {
        fprintf(stderr, "Error reading header from %s\n", file_name);
        fclose(file);
        return -1;
    }

    ul i = 0;
    while (i < max_points && fgets(line, sizeof(line), file) != NULL)
    {
        usize len = strlen(line);
        if (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
        {
            line[--len] = '\0';
        }
        if (len == 0)
        {
            continue;
        }

        char *p = line;
        char *endptr = NULL;

        f64 ra = strtod(p, &endptr);
        if (endptr == p)
        {
            fprintf(stderr, "Parse error (RA) on line %lu in %s: '%s'\n", (ul)i + 2, file_name, line);
            fclose(file);
            return -1;
        }

        p = endptr;
        while (*p == '\t' || *p == ' ' || *p == ',')
        {
            ++p;
        }

        f64 dec = strtod(p, &endptr);
        if (endptr == p)
        {
            fprintf(stderr, "Parse error (DEC) on line %lu in %s: '%s'\n", (ul)i + 2, file_name, line);
            fclose(file);
            return -1;
        }

        data_points_location[i].right_ascension = ra;
        data_points_location[i].declination = dec;

        ++i;
    }

    fclose(file);
    return (usize)i;
}

internal i32
app_init(app_state_t *app_state)
{
    *app_state = (app_state_t){
        .window_width = INITIAL_WINDOW_WIDTH,
        .window_height = INITIAL_WINDOW_HEIGHT,
        .cpu_memory_allocated = 0L,
        .debug = false,
        .data_is_loaded = false,
        .is_paused = false,
        .cursor_enabled = true,

        .data_points_a = NULL,
        .data_points_b = NULL,

        .main_camera = (Camera3D){
            .position = (Vector3){0.0f, 0.0f, 0.0f},
            .target = (Vector3){0.0f, 0.0f, 0.0f},
            .up = (Vector3){0.0f, 1.0f, 0.0f},
            .fovy = 85.0f,
            .projection = CAMERA_PERSPECTIVE,
        },

        .camera_zoom = 0.8f * PI,
        .camera_yaw = 45.80f,
        .camera_pitch = 42.12f,
        .camera_direction = (Vector3){0},

        .custom_shader = (Shader){0},
        .material_instance = (Material){0},
        .sphere_mesh = (Mesh){0},
        .earth_model = (Model){0},

        .matrix_transforms_a = NULL,
        .matrix_transforms_b = NULL,
        .data_to_draw = DRAW_DATA_ALL,
    };

    i32 app_read_input_data_result = app_read_input_data(app_state);
    if (app_read_input_data_result != 0)
    {
        fprintf(stderr, "ERROR: Could not perform app_read_input_data.\n");
        return 1;
    }

    printf("\tHello from raylib_galaxy_application!\n\n");

    i32 matrix_gpu_upload_result = upload_matrix_transforms_to_gpu(app_state);
    if (matrix_gpu_upload_result != 0)
    {
        fprintf(stderr, "ERROR: Could not upload matrix transforms to the GPU.\n");
        return 1;
    }

    i32 platform_init_result = app_init_platform(app_state);
    if (platform_init_result != 0)
    {
        fprintf(stderr, "ERROR: Could not initialize the platform API (Raylib)\n");
        return 1;
    }

    app_state->main_font = LoadFontEx("./assets/fonts/retro-pixel-arcade.ttf", 128, 0, 250);

    app_state->sphere_mesh = GenMeshSphere(0.2f, 4, 4);

    i32 shader_init_result = app_init_shaders(app_state);
    if (shader_init_result != 0)
    {
        fprintf(stderr, "ERROR: Could not initialize the shader(s)\n");
        return 1;
    }

    printf("\n\tMemory usage before we start the main program loop\n");
    print_memory_usage(app_state);

    app_state->earth_model = LoadModel("./assets/Earth_1_12756.glb");
    Matrix earch_scale_matrix = MatrixScale(0.01f, 0.01f, 0.01f);
    app_state->earth_model.transform = MatrixMultiply(app_state->earth_model.transform, earch_scale_matrix);

    return 0;
}

internal i32
app_read_input_data(app_state_t *app_state)
{
    app_state->data_points_a = (arcmin_data_t *)calloc(MAX_DATA_POINTS, sizeof(arcmin_data_t));
    if (app_state->data_points_a == NULL)
    {
        fprintf(stderr, "Error allocating memory for data_points_a!\n");
        return 1;
    }
    app_state->cpu_memory_allocated += (usize)MAX_DATA_POINTS * sizeof(arcmin_data_t);

    app_state->data_points_b = (arcmin_data_t *)calloc(MAX_DATA_POINTS, sizeof(arcmin_data_t));
    if (app_state->data_points_b == NULL)
    {
        fprintf(stderr, "Error allocating memory for data_points_b!\n");
        free(app_state->data_points_a);
        app_state->cpu_memory_allocated -= (usize)MAX_DATA_POINTS * sizeof(arcmin_data_t);
        app_state->data_points_a = NULL;
        return 1;
    }
    app_state->cpu_memory_allocated += (usize)MAX_DATA_POINTS * sizeof(arcmin_data_t);

    usize count_a = read_input_data_from_file(data_a_filename, app_state->data_points_a, MAX_DATA_POINTS);
    if (count_a < 0)
    {
        fprintf(stderr, "Failed reading %s\n", data_a_filename);
        return 1;
    }

    usize count_b = read_input_data_from_file(data_b_filename, app_state->data_points_b, MAX_DATA_POINTS);
    if (count_b < 0)
    {
        fprintf(stderr, "Failed reading %s\n", data_b_filename);
        return 1;
    }

    if ((usize)count_a != (usize)count_b)
    {
        fprintf(stderr, "Warning: datasets A and B have different row counts: %zd vs %zd\n", count_a, count_b);
    }

    app_state->data_point_count = (ul)MIN((usize)count_a, (usize)count_b); // store the actual amount available

    if (app_state->data_point_count == 0)
    {
        fprintf(stderr, "No data loaded from files.\n");
        return 1;
    }

    app_state->data_is_loaded = true;
    return 0;
}

internal i32
initialize_transforms_course_data(app_state_t *app_state)
{
    for (ul i = 0; i < app_state->data_point_count; ++i)
    {
        // data_points_a real galaxies
        {
            const f64 right_ascension_rad = (app_state->data_points_a[i].right_ascension / 60.0) * DEG2RAD;
            const f64 declination_rad = (app_state->data_points_a[i].declination / 60.0) * DEG2RAD;
            const f64 radius = 50.0;
            const f64 x = radius * cos(right_ascension_rad) * cos(declination_rad);
            const f64 y = radius * sin(declination_rad);
            const f64 z = radius * sin(right_ascension_rad) * cos(declination_rad);

            app_state->matrix_transforms_a[i] = MatrixIdentity();
            app_state->matrix_transforms_a[i] = MatrixMultiply(app_state->matrix_transforms_a[i], MatrixScale(0.1f, 0.1f, 0.1f));
            app_state->matrix_transforms_a[i] = MatrixMultiply(app_state->matrix_transforms_a[i], MatrixTranslate((f32)x, (f32)y, (f32)z));
        }

        // data_points_b uniformly distributed (galaxies)
        {
            const f64 right_ascension_rad = (app_state->data_points_b[i].right_ascension / 60.0) * DEG2RAD;
            const f64 declination_rad = (app_state->data_points_b[i].declination / 60.0) * DEG2RAD;
            const f64 radius = 50.0;
            const f64 x = radius * cos(right_ascension_rad) * cos(declination_rad);
            const f64 y = radius * sin(declination_rad);
            const f64 z = radius * sin(right_ascension_rad) * cos(declination_rad);

            app_state->matrix_transforms_b[i] = MatrixIdentity();
            app_state->matrix_transforms_b[i] = MatrixMultiply(app_state->matrix_transforms_b[i], MatrixScale(0.1f, 0.1f, 0.1f));
            app_state->matrix_transforms_b[i] = MatrixMultiply(app_state->matrix_transforms_b[i], MatrixTranslate((f32)x, (f32)y, (f32)z));
        }
    }

    return 0;
}

internal i32
upload_matrix_transforms_to_gpu(app_state_t *app_state)
{
    app_state->matrix_transforms_a = (Matrix *)calloc(MAX_DATA_POINTS, sizeof(Matrix));
    if (app_state->matrix_transforms_a == NULL)
    {
        fprintf(stderr, "ERROR: Could not allocate matrix_transforms_a.\n");
        return 1;
    }
    app_state->cpu_memory_allocated += MAX_DATA_POINTS * sizeof(Matrix);

    app_state->matrix_transforms_b = (Matrix *)calloc(MAX_DATA_POINTS, sizeof(Matrix));
    if (app_state->matrix_transforms_b == NULL)
    {
        fprintf(stderr, "ERROR: Could not allocate matrix_transforms_b.\n");
        return 1;
    }
    app_state->cpu_memory_allocated += MAX_DATA_POINTS * sizeof(Matrix);

    i32 course_data_init_result = initialize_transforms_course_data(app_state);
    if (course_data_init_result != 0)
    {
        fprintf(stderr, "ERROR: Could not initialize course data matrix transforms.\n");
        return 1;
    }

    return 0;
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
        i32 ambient_location = GetShaderLocation(app_state->custom_shader, "ambient");
        f32 ambient_value[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        SetShaderValue(app_state->custom_shader, ambient_location, ambient_value, SHADER_UNIFORM_VEC4);

        i32 color_diffuse_loc = GetShaderLocation(app_state->custom_shader, "colorDiffuse");
        f64 diffuse_value[4] = {1.0, 1.0, 1.0, 1.0};
        SetShaderValue(app_state->custom_shader, color_diffuse_loc, &diffuse_value, SHADER_UNIFORM_VEC4);

        // The sun shining on the earth
        CreateLight(LIGHT_DIRECTIONAL, (Vector3){1000.0f, 1000.0f, 0.0f}, Vector3Zero(), WHITE, app_state->custom_shader);

        // @Note(Victor): We add more lights to the scene to better show the colors of the galaxies
        CreateLight(LIGHT_DIRECTIONAL, (Vector3){-1000.0f, -1000.0f, 0.0f}, Vector3Zero(), WHITE, app_state->custom_shader);
        CreateLight(LIGHT_DIRECTIONAL, (Vector3){0.0f, 0.0f, 1000.0f}, Vector3Zero(), WHITE, app_state->custom_shader);
        CreateLight(LIGHT_DIRECTIONAL, (Vector3){0.0f, 0.0f, -1000.0f}, Vector3Zero(), WHITE, app_state->custom_shader);

        // We also add a poi32 light at the center of the earth
        CreateLight(LIGHT_POINT, (Vector3){0.0f, 0.0f, 0.0f}, Vector3Zero(), WHITE, app_state->custom_shader);
    }

    // Material
    {
        Material galaxy_material = LoadMaterialDefault();
        galaxy_material.shader = app_state->custom_shader;

        galaxy_material.maps[MATERIAL_MAP_DIFFUSE].texture = LoadTexture("./assets/images/galaxy_test_texture_diffuse.png");
        galaxy_material.maps[MATERIAL_MAP_SPECULAR].texture = LoadTexture("./assets/images/galaxy_test_texture_specular.png");

        galaxy_material.maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
        galaxy_material.maps[MATERIAL_MAP_SPECULAR].value = 1.0f;

        f32 shininess = 32.0f;
        SetShaderValue(galaxy_material.shader, GetShaderLocation(galaxy_material.shader, "shininess"), &shininess, SHADER_UNIFORM_FLOAT);

        app_state->material_instance = galaxy_material;
        app_state->material_instance.shader = app_state->custom_shader;
        app_state->material_instance.maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
    }

    return 0;
}

internal i32
app_init_platform(app_state_t *app_state)
{
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);

    InitWindow(app_state->window_width, app_state->window_height, "galaxy_visuazation_raylib");

    SetWindowIcon(LoadImage("./assets/images/app_icon.png"));

    return 0;
}
