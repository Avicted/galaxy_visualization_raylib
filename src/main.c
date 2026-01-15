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

// Redshift galaxy data from seyfert.dat catalog
typedef struct
{
    char name[16];       // Galaxy name
    f64 right_ascension; // In degrees (converted from H:M:S)
    f64 declination;     // In degrees (converted from D:M:S)
    f64 helio_velocity;  // Heliocentric velocity in km/s
    f64 b_magnitude;     // B magnitude
} redshift_galaxy_t;

typedef enum
{
    DRAW_DATA_A,
    DRAW_DATA_B,
    DRAW_DATA_ALL,
    DRAW_DATA_REDSHIFT, // Seyfert redshift data with true 3D depth
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
    bool show_help;
    f64 fps_smoothed;
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
    Mesh cube_mesh;
    Model earth_model;

    // Batch rendering
    Material material_instance;
    Material redshift_material;
    Matrix *matrix_transforms_a;
    Matrix *matrix_transforms_b;

    // Redshift galaxy data (Seyfert catalog)
    redshift_galaxy_t *redshift_galaxies;
    Matrix *matrix_transforms_redshift;
    Color *redshift_galaxy_colors; // Color per galaxy based on velocity/redshift
    ul redshift_galaxy_count;
} app_state_t;

// Constants ---------------------------------------------------------------------
global_variable const ul MAX_DATA_POINTS = 100000UL;
global_variable const ul MAX_REDSHIFT_GALAXIES = 2000UL;
global_variable const char *data_a_filename = "./input_data/data_100k_arcmin.txt";
global_variable const char *data_b_filename = "./input_data/flat_100k_arcmin.txt";
global_variable const char *redshift_data_filename = "./input_data/redshift_input_data/seyfert.dat";

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
internal usize read_redshift_data_from_file(const char *file_name, redshift_galaxy_t *galaxies, ul max_galaxies);
internal i32 upload_matrix_transforms_to_gpu(app_state_t *app_state);
internal i32 initialize_transforms_course_data(app_state_t *app_state);
internal i32 initialize_transforms_redshift_data(app_state_t *app_state);
internal f64 calculate_distance_from_velocity(f64 velocity_km_s);
internal void parse_input_args(app_state_t *app_state, i32 argc, char **argv);
internal inline void handle_window_resize(app_state_t *app_state);
internal inline void rotate_camera_around_origo(app_state_t *app_state, f64 dt);
internal void print_memory_usage(app_state_t *app_state);

internal const char *format_u64_thousands_dots(u64 value);

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

    if (IsKeyPressed(KEY_FOUR))
    {
        app_state->data_to_draw = DRAW_DATA_REDSHIFT;
    }

    if (IsKeyPressed(KEY_R))
    {
        app_state->is_paused = !app_state->is_paused;
        printf("\tis_paused: %s\n", app_state->is_paused ? "true" : "false");
    }

    if (IsKeyPressed(KEY_H))
    {
        app_state->show_help = !app_state->show_help;
    }

    rotate_camera_around_origo(app_state, dt);

    f64 scroll = GetMouseWheelMove();
    if (scroll != 0.0f)
    {
        const f64 zoom_change = -32.0f;
        app_state->camera_zoom = Clamp(app_state->camera_zoom + scroll * zoom_change * dt, 0.5f, 5.0f);
    }

    // Smooth FPS calculation for stable display
    if (dt > 0.0)
    {
        f64 fps_inst = 1.0 / dt;
        if (app_state->fps_smoothed <= 0.0)
        {
            app_state->fps_smoothed = fps_inst;
        }
        else
        {
            // Exponential moving average for stability
            const f64 alpha = 0.1;
            app_state->fps_smoothed = app_state->fps_smoothed * (1.0 - alpha) + fps_inst * alpha;
        }
    }
}

internal const char *
format_u64_thousands_dots(u64 value)
{
    // Returns a pointer to a static buffer (rotating) containing "100.000"-style formatting.
    local_persist char buffers[4][32];
    local_persist i32 buffer_index = 0;

    char *buf = buffers[buffer_index++ & 3];

    char *out = buf + 31;
    *out = '\0';

    if (value == 0)
    {
        *--out = '0';
        return out;
    }

    i32 group = 0;
    while (value > 0)
    {
        if (group == 3)
        {
            *--out = '.';
            group = 0;
        }

        *--out = (char)('0' + (value % 10));
        value /= 10;
        group++;
    }

    return out;
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

    if (app_state->data_to_draw == DRAW_DATA_REDSHIFT && app_state->redshift_galaxy_count > 0)
    {
        // Draw each galaxy as a simple colored cube - fast and visible
        for (ul i = 0; i < app_state->redshift_galaxy_count; ++i)
        {
            // Extract position from transformation matrix
            Vector3 pos = {
                app_state->matrix_transforms_redshift[i].m12,
                app_state->matrix_transforms_redshift[i].m13,
                app_state->matrix_transforms_redshift[i].m14};

            // Extract scale from matrix (m0 = scaleX, assumes uniform scale)
            f32 size = app_state->matrix_transforms_redshift[i].m0;
            Color base_color = app_state->redshift_galaxy_colors[i];

            // Brighten colors significantly for visibility
            Color bright_color = {
                (u8)fmin(255, base_color.r + 80),
                (u8)fmin(255, base_color.g + 80),
                (u8)fmin(255, base_color.b + 80),
                255};

            // Draw as cube - much faster than sphere
            DrawCube(pos, size, size, size, bright_color);
        }
    }

    EndMode3D();

    // UI -----------------------------------------------------------------------------
    const Color PANEL_BG = {20, 20, 30, 200};
    const Color PANEL_BORDER = {60, 60, 80, 255};
    const Color TEXT_DIM = {180, 180, 180, 255};
    const Color ACCENT_CYAN = {100, 220, 255, 255};
    const Color ACCENT_ORANGE = {255, 160, 80, 255};
    const Color ACCENT_GREEN = {100, 255, 150, 255};
    const Color ACCENT_PURPLE = {200, 130, 255, 255};

    // Top-left: Current mode and FPS
    {
        const i32 panel_w = 340;
        const i32 panel_h = 100;
        DrawRectangle(8, 8, panel_w, panel_h, PANEL_BG);
        DrawRectangleLines(8, 8, panel_w, panel_h, PANEL_BORDER);

        // Dataset name - shorter names
        const char *dataset_names[] = {"Real Data", "Uniform", "Both", "Seyfert 3D"};
        Color dataset_colors[] = {ACCENT_CYAN, ACCENT_ORANGE, WHITE, ACCENT_PURPLE};
        DrawTextEx(app_state->main_font, dataset_names[app_state->data_to_draw],
                   (Vector2){16, 12}, 32, 1, dataset_colors[app_state->data_to_draw]);

        // FPS counter on its own line
        DrawTextEx(app_state->main_font, TextFormat("FPS: %.0f", app_state->fps_smoothed),
                   (Vector2){16, 52}, 24, 1, TEXT_DIM);

        // Galaxy count for redshift mode on a new line to avoid overlap
        ul galaxy_count = 0UL;
        if (app_state->data_to_draw == DRAW_DATA_REDSHIFT)
        {
            galaxy_count = app_state->redshift_galaxy_count;
        }
        else if (app_state->data_to_draw == DRAW_DATA_A || app_state->data_to_draw == DRAW_DATA_B || app_state->data_to_draw == DRAW_DATA_ALL)
        {
            galaxy_count = app_state->data_point_count * ((app_state->data_to_draw == DRAW_DATA_ALL) ? 2 : 1);
        }

        DrawTextEx(app_state->main_font, TextFormat("Galaxies: %s", format_u64_thousands_dots((u64)galaxy_count)),
                   (Vector2){16, 74}, 24, 1, TEXT_DIM);
    }

    // Top-right: Mode indicator and color legend
    {
        const i32 panel_width = 260;
        const i32 panel_x = app_state->window_width - panel_width - 8;

        // Mode badge
        const char *mode_text = app_state->is_paused ? "Free Look" : "Auto";
        Color mode_color = app_state->is_paused ? ACCENT_PURPLE : ACCENT_GREEN;
        i32 mode_panel_height = 32;

        DrawRectangle(panel_x, 8, panel_width, mode_panel_height, PANEL_BG);
        DrawRectangleLines(panel_x, 8, panel_width, mode_panel_height, mode_color);
        DrawTextEx(app_state->main_font, mode_text,
                   (Vector2){(f32)(panel_x + 12), 12}, 24, 1, mode_color);

        // Color legend for redshift mode
        if (app_state->data_to_draw == DRAW_DATA_REDSHIFT)
        {
            const i32 legend_y = 52;
            const i32 legend_height = 110;

            DrawRectangle(panel_x, legend_y, panel_width, legend_height, PANEL_BG);
            DrawRectangleLines(panel_x, legend_y, panel_width, legend_height, PANEL_BORDER);

            DrawTextEx(app_state->main_font, "Velocity (km/s)",
                       (Vector2){(f32)(panel_x + 10), (f32)(legend_y + 8)}, 24, 1, TEXT_DIM);

            // Gradient bar
            const i32 bar_x = panel_x + 10;
            const i32 bar_y = legend_y + 40;
            const i32 bar_width = panel_width - 20;
            const i32 bar_height = 16;
            const i32 num_segments = 40;
            const f32 seg_width = (f32)bar_width / (f32)num_segments;

            for (i32 seg = 0; seg < num_segments; seg++)
            {
                f64 t = (f64)seg / (f64)(num_segments - 1);
                Color seg_color;

                if (t < 0.33)
                {
                    f64 s = t / 0.33;
                    seg_color.r = (u8)(100 + s * 155);
                    seg_color.g = (u8)(255 - s * 25);
                    seg_color.b = (u8)(255 - s * 200);
                    seg_color.a = 255;
                }
                else if (t < 0.66)
                {
                    f64 s = (t - 0.33) / 0.33;
                    seg_color.r = 255;
                    seg_color.g = (u8)(230 - s * 100);
                    seg_color.b = (u8)(55 - s * 35);
                    seg_color.a = 255;
                }
                else
                {
                    f64 s = (t - 0.66) / 0.34;
                    seg_color.r = (u8)(255 - s * 55);
                    seg_color.g = (u8)(130 - s * 90);
                    seg_color.b = (u8)(20 - s * 10);
                    seg_color.a = 255;
                }

                DrawRectangle((i32)(bar_x + seg * seg_width), bar_y, (i32)(seg_width + 1), bar_height, seg_color);
            }
            DrawRectangleLines(bar_x, bar_y, bar_width, bar_height, PANEL_BORDER);

            // Labels
            DrawTextEx(app_state->main_font, "1k",
                       (Vector2){(f32)bar_x, (f32)(bar_y + bar_height + 8)}, 24, 1, TEXT_DIM);
            DrawTextEx(app_state->main_font, "86k",
                       (Vector2){(f32)(bar_x + bar_width - 40), (f32)(bar_y + bar_height + 8)}, 24, 1, TEXT_DIM);
        }
    }

    // Bottom center: Mode toggle hint
    {
        const char *hint_text = app_state->is_paused ? "R - Return to Auto Orbit" : "R - Enter Free Look";
        Color hint_color = app_state->is_paused ? ACCENT_PURPLE : ACCENT_GREEN;
        Vector2 text_size = MeasureTextEx(app_state->main_font, hint_text, 24, 2);
        f32 hint_x = (app_state->window_width - text_size.x) / 2.0f;
        f32 hint_y = app_state->window_height - 50.0f;

        DrawRectangle((i32)(hint_x - 12), (i32)(hint_y - 8), (i32)(text_size.x + 24), 40, PANEL_BG);
        DrawRectangleLines((i32)(hint_x - 12), (i32)(hint_y - 8), (i32)(text_size.x + 24), 40, hint_color);
        DrawTextEx(app_state->main_font, hint_text, (Vector2){hint_x, hint_y}, 24, 2, hint_color);
    }

    // Help panel (togglable with H)
    const i32 help_x = 8;
    const i32 help_y = 120;
    if (app_state->show_help)
    {
        const i32 help_width = 340;
        i32 help_height = app_state->is_paused ? 200 : 170;

        DrawRectangle(help_x, help_y, help_width, help_height, PANEL_BG);
        DrawRectangleLines(help_x, help_y, help_width, help_height, PANEL_BORDER);

        i32 line_y = help_y + 10;
        const i32 line_spacing = 24;

        DrawTextEx(app_state->main_font, "Controls", (Vector2){(f32)(help_x + 12), (f32)line_y}, 24, 2, WHITE);
        line_y += line_spacing + 4;

        DrawTextEx(app_state->main_font, "1-4  Dataset", (Vector2){(f32)(help_x + 12), (f32)line_y}, 24, 1, TEXT_DIM);
        line_y += line_spacing;
        DrawTextEx(app_state->main_font, "R    Camera mode", (Vector2){(f32)(help_x + 12), (f32)line_y}, 24, 1, TEXT_DIM);
        line_y += line_spacing;
        DrawTextEx(app_state->main_font, "H    Toggle help", (Vector2){(f32)(help_x + 12), (f32)line_y}, 24, 1, TEXT_DIM);
        line_y += line_spacing;
        DrawTextEx(app_state->main_font, "F11  Fullscreen", (Vector2){(f32)(help_x + 12), (f32)line_y}, 24, 1, TEXT_DIM);
        line_y += line_spacing;
        DrawTextEx(app_state->main_font, "Scroll  Zoom", (Vector2){(f32)(help_x + 12), (f32)line_y}, 24, 1, TEXT_DIM);

        if (app_state->is_paused)
        {
            line_y += line_spacing + 6;
            DrawTextEx(app_state->main_font, "WASD+Mouse Shift/Space", (Vector2){(f32)(help_x + 12), (f32)line_y}, 24, 1, ACCENT_PURPLE);
        }
    }
    else
    {
        // Minimal help hint
        DrawTextEx(app_state->main_font, "H - Help", (Vector2){16, help_y}, 24, 1, TEXT_DIM);
    }

    // Dataset legend (compact, bottom-left)
    {
        const i32 legend_x = 8;
        const i32 legend_width = 240;
        const i32 legend_height = 130;
        const i32 legend_y = app_state->window_height - legend_height - 8;

        DrawRectangle(legend_x, legend_y, legend_width, legend_height, PANEL_BG);
        DrawRectangleLines(legend_x, legend_y, legend_width, legend_height, PANEL_BORDER);

        const i32 legend_text_x = legend_x + 8;
        const i32 legend_text_y = legend_y + 10;
        const i32 legend_line_spacing = 28;

        DrawTextEx(app_state->main_font, "1 Real (blue)",
                   (Vector2){(f32)legend_text_x, (f32)(legend_text_y + 0 * legend_line_spacing)}, 24, 1, ACCENT_CYAN);
        DrawTextEx(app_state->main_font, "2 Uniform",
                   (Vector2){(f32)legend_text_x, (f32)(legend_text_y + 1 * legend_line_spacing)}, 24, 1, ACCENT_ORANGE);
        DrawTextEx(app_state->main_font, "3 Both",
                   (Vector2){(f32)legend_text_x, (f32)(legend_text_y + 2 * legend_line_spacing)}, 24, 1, TEXT_DIM);
        DrawTextEx(app_state->main_font, "4 Seyfert",
                   (Vector2){(f32)legend_text_x, (f32)(legend_text_y + 3 * legend_line_spacing)}, 24, 1, ACCENT_PURPLE);
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
    if (app_state->cube_mesh.vboId)
    {
        UnloadMesh(app_state->cube_mesh);
    }
    if (app_state->material_instance.shader.id)
    {
        UnloadShader(app_state->material_instance.shader);
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
    if (app_state->redshift_galaxies)
    {
        free(app_state->redshift_galaxies);
        app_state->cpu_memory_allocated -= (usize)MAX_REDSHIFT_GALAXIES * sizeof(redshift_galaxy_t);
    }
    if (app_state->matrix_transforms_redshift)
    {
        free(app_state->matrix_transforms_redshift);
        app_state->cpu_memory_allocated -= app_state->redshift_galaxy_count * sizeof(Matrix);
    }
    if (app_state->redshift_galaxy_colors)
    {
        free(app_state->redshift_galaxy_colors);
        app_state->cpu_memory_allocated -= app_state->redshift_galaxy_count * sizeof(Color);
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

// Calculate distance from heliocentric velocity using Hubble's Law
// Returns distance in visualization units (scaled for rendering)
internal f64
calculate_distance_from_velocity(f64 velocity_km_s)
{
    if (velocity_km_s <= 0.0)
        return 200.0; // Minimum distance well outside earth

    // Apply power scaling to spread out the distribution nicely
    // Velocities range from ~1000 to ~90000 km/s
    // We want distances from ~50 to ~200 render units (much further from Earth)
    f64 normalized = (velocity_km_s - 500.0) / 90000.0; // 0 to ~1
    normalized = fmax(0.0, fmin(normalized, 1.0));

    // Use square root for gentler compression (better spread)
    f64 render_distance = 200.0 + sqrt(normalized) * 600.0;

    return render_distance;
}

// Calculate color based on redshift velocity (astronomical redshift coloring)
// Nearby galaxies appear blue-white, distant ones appear progressively redder
internal Color
calculate_redshift_color(f64 velocity_km_s)
{
    // Normalize velocity to 0-1 range
    // ~1000 km/s = nearby (cyan/blue), ~90000 km/s = far (deep red)
    f64 t = (velocity_km_s - 1000.0) / 85000.0;
    t = fmax(0.0, fmin(t, 1.0));

    // Color gradient: cyan -> green-yellow -> orange -> red
    // This mimics actual redshift where light shifts toward red at higher velocities
    Color color;

    if (t < 0.33)
    {
        // Cyan to green-yellow (nearby galaxies)
        f64 s = t / 0.33;
        color.r = (u8)(100 + s * 155); // 100 -> 255
        color.g = (u8)(255 - s * 25);  // 255 -> 230
        color.b = (u8)(255 - s * 200); // 255 -> 55
        color.a = 255;
    }
    else if (t < 0.66)
    {
        // Yellow to orange
        f64 s = (t - 0.33) / 0.33;
        color.r = 255;
        color.g = (u8)(230 - s * 100); // 230 -> 130
        color.b = (u8)(55 - s * 35);   // 55 -> 20
        color.a = 255;
    }
    else
    {
        // Orange to deep red (most distant)
        f64 s = (t - 0.66) / 0.34;
        color.r = (u8)(255 - s * 55); // 255 -> 200
        color.g = (u8)(130 - s * 90); // 130 -> 40
        color.b = (u8)(20 - s * 10);  // 20 -> 10
        color.a = 255;
    }

    return color;
}

// Read redshift galaxy data from seyfert.dat catalog file
// Uses whitespace-delimited parsing since the format has variable spacing
internal usize
read_redshift_data_from_file(const char *file_name, redshift_galaxy_t *galaxies, ul max_galaxies)
{
    FILE *file = fopen(file_name, "r");
    if (file == NULL)
    {
        fprintf(stderr, "Error opening redshift file: %s\n", file_name);
        return 0;
    }

    char line[256];
    ul galaxy_count = 0;
    ul line_number = 0;

    // Skip header lines (first 14 lines are comments/header)
    while (line_number < 14 && fgets(line, sizeof(line), file) != NULL)
    {
        line_number++;
    }

    // Parse data lines using whitespace-delimited approach
    // Format: NAME RA DEC BMAG VELOCITY ...
    // Where RA=HHMMSS.S, DEC=±DDMMSS
    while (galaxy_count < max_galaxies && fgets(line, sizeof(line), file) != NULL)
    {
        line_number++;
        usize len = strlen(line);

        // Skip empty or too short lines
        if (len < 30)
            continue;

        // Skip lines starting with dashes (separators)
        if (line[0] == '-')
            continue;
        if (line[0] == '\n' || line[0] == '\r')
            continue;

        // Tokenize the line by whitespace
        char line_copy[256];
        strncpy(line_copy, line, 255);
        line_copy[255] = '\0';

        char *tokens[20] = {0};
        i32 token_count = 0;
        char *token = strtok(line_copy, " \t\n\r");
        while (token != NULL && token_count < 20)
        {
            tokens[token_count++] = token;
            token = strtok(NULL, " \t\n\r");
        }

        // Need at least 5 tokens: NAME, RA, DEC, BMAG, VELOCITY
        if (token_count < 5)
            continue;

        // Token 0: Galaxy name
        strncpy(galaxies[galaxy_count].name, tokens[0], 15);
        galaxies[galaxy_count].name[15] = '\0';

        // Token 1: RA in HHMMSS.S format
        char *ra_str = tokens[1];
        usize ra_len = strlen(ra_str);
        if (ra_len < 6)
            continue;

        // Parse HHMMSS.S
        char ra_h_str[3] = {ra_str[0], ra_str[1], '\0'};
        char ra_m_str[3] = {ra_str[2], ra_str[3], '\0'};
        char ra_s_str[8] = {0};
        strncpy(ra_s_str, ra_str + 4, 7);

        i32 ra_hours = atoi(ra_h_str);
        i32 ra_minutes = atoi(ra_m_str);
        f64 ra_seconds = strtod(ra_s_str, NULL);

        f64 ra_decimal_hours = (f64)ra_hours + (f64)ra_minutes / 60.0 + ra_seconds / 3600.0;
        galaxies[galaxy_count].right_ascension = ra_decimal_hours * 15.0; // Convert to degrees

        // Token 2: DEC in ±DDMMSS format
        char *dec_str = tokens[2];
        usize dec_len = strlen(dec_str);
        if (dec_len < 5)
            continue;

        bool is_negative = (dec_str[0] == '-');
        i32 dec_start = (dec_str[0] == '-' || dec_str[0] == '+') ? 1 : 0;

        char dec_d_str[3] = {dec_str[dec_start], dec_str[dec_start + 1], '\0'};
        char dec_m_str[3] = {dec_str[dec_start + 2], dec_str[dec_start + 3], '\0'};
        char dec_s_str[3] = {0};
        if (dec_len >= (usize)(dec_start + 6))
        {
            dec_s_str[0] = dec_str[dec_start + 4];
            dec_s_str[1] = dec_str[dec_start + 5];
        }

        i32 dec_degrees = atoi(dec_d_str);
        i32 dec_minutes = atoi(dec_m_str);
        i32 dec_seconds = atoi(dec_s_str);

        f64 dec_decimal = (f64)dec_degrees + (f64)dec_minutes / 60.0 + (f64)dec_seconds / 3600.0;
        if (is_negative)
        {
            dec_decimal = -dec_decimal;
        }
        galaxies[galaxy_count].declination = dec_decimal;

        // Token 3: B magnitude
        galaxies[galaxy_count].b_magnitude = strtod(tokens[3], NULL);

        // Token 4: Heliocentric velocity in km/s
        i32 velocity = atoi(tokens[4]);

        // Skip galaxies without valid velocity data (need reasonable cosmological velocity)
        // Typical redshift galaxies have velocities > 1000 km/s
        if (velocity <= 500)
        {
            continue;
        }

        galaxies[galaxy_count].helio_velocity = (f64)velocity;

        galaxy_count++;
    }

    fclose(file);
    printf("\tLoaded %lu redshift galaxies from %s\n", galaxy_count, file_name);
    return galaxy_count;
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
        .show_help = true,
        .fps_smoothed = 0.0,

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
        .redshift_material = (Material){0},
        .sphere_mesh = (Mesh){0},
        .cube_mesh = (Mesh){0},
        .earth_model = (Model){0},

        .matrix_transforms_a = NULL,
        .matrix_transforms_b = NULL,
        .data_to_draw = DRAW_DATA_ALL,

        .redshift_galaxies = NULL,
        .matrix_transforms_redshift = NULL,
        .redshift_galaxy_colors = NULL,
        .redshift_galaxy_count = 0,
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

    // app_state->main_font = LoadFontEx("./assets/fonts/retro-pixel-arcade.ttf", 128, 0, 250);
    app_state->main_font = LoadFontEx("./assets/fonts/Perfect DOS VGA 437.ttf", 128, 0, 250);

    app_state->sphere_mesh = GenMeshSphere(0.2f, 4, 4);
    app_state->cube_mesh = GenMeshCube(1.0f, 1.0f, 1.0f);

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

    // Load redshift galaxy data from Seyfert catalog
    app_state->redshift_galaxies = (redshift_galaxy_t *)calloc(MAX_REDSHIFT_GALAXIES, sizeof(redshift_galaxy_t));
    if (app_state->redshift_galaxies == NULL)
    {
        fprintf(stderr, "Error allocating memory for redshift_galaxies!\n");
        return 1;
    }
    app_state->cpu_memory_allocated += (usize)MAX_REDSHIFT_GALAXIES * sizeof(redshift_galaxy_t);

    usize redshift_count = read_redshift_data_from_file(redshift_data_filename, app_state->redshift_galaxies, MAX_REDSHIFT_GALAXIES);
    app_state->redshift_galaxy_count = (ul)redshift_count;

    if (app_state->redshift_galaxy_count == 0)
    {
        fprintf(stderr, "Warning: No redshift galaxies loaded from %s\n", redshift_data_filename);
    }
    else
    {
        printf("\tSuccessfully loaded %lu redshift galaxies with 3D depth information\n", app_state->redshift_galaxy_count);
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
initialize_transforms_redshift_data(app_state_t *app_state)
{
    for (ul i = 0; i < app_state->redshift_galaxy_count; ++i)
    {
        redshift_galaxy_t *galaxy = &app_state->redshift_galaxies[i];

        // Convert celestial coordinates (in degrees) to radians
        const f64 right_ascension_rad = galaxy->right_ascension * DEG2RAD;
        const f64 declination_rad = galaxy->declination * DEG2RAD;

        // Calculate distance from velocity using Hubble's Law with log scaling
        const f64 radius = calculate_distance_from_velocity(galaxy->helio_velocity);

        // Spherical to Cartesian conversion
        // RA maps to azimuthal angle, DEC maps to elevation
        const f64 x = radius * cos(declination_rad) * cos(right_ascension_rad);
        const f64 y = radius * sin(declination_rad);
        const f64 z = radius * cos(declination_rad) * sin(right_ascension_rad);

        // Create transformation matrix for instanced rendering
        app_state->matrix_transforms_redshift[i] = MatrixIdentity();

        // Scale based on magnitude - brighter galaxies (lower magnitude) appear larger
        // Small scale for point-like appearance
        f64 magnitude_scale = 0.6;
        if (galaxy->b_magnitude > 0.0 && galaxy->b_magnitude < 20.0)
        {
            // Scale factor: brighter = larger (magnitude 12 -> scale ~0.8, magnitude 17 -> scale ~0.4)
            magnitude_scale = 1.0 / (galaxy->b_magnitude / 12.0);
            magnitude_scale = fmax(0.3, fmin(magnitude_scale, 1.2));
        }

        app_state->matrix_transforms_redshift[i] = MatrixMultiply(
            app_state->matrix_transforms_redshift[i],
            MatrixScale((f32)magnitude_scale, (f32)magnitude_scale, (f32)magnitude_scale));
        app_state->matrix_transforms_redshift[i] = MatrixMultiply(
            app_state->matrix_transforms_redshift[i],
            MatrixTranslate((f32)x, (f32)y, (f32)z));

        // Calculate color based on redshift (velocity)
        app_state->redshift_galaxy_colors[i] = calculate_redshift_color(galaxy->helio_velocity);
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

    // Allocate and initialize redshift galaxy transforms
    if (app_state->redshift_galaxy_count > 0)
    {
        app_state->matrix_transforms_redshift = (Matrix *)calloc(app_state->redshift_galaxy_count, sizeof(Matrix));
        if (app_state->matrix_transforms_redshift == NULL)
        {
            fprintf(stderr, "ERROR: Could not allocate matrix_transforms_redshift.\n");
            return 1;
        }
        app_state->cpu_memory_allocated += app_state->redshift_galaxy_count * sizeof(Matrix);

        // Allocate color array for per-galaxy coloring
        app_state->redshift_galaxy_colors = (Color *)calloc(app_state->redshift_galaxy_count, sizeof(Color));
        if (app_state->redshift_galaxy_colors == NULL)
        {
            fprintf(stderr, "ERROR: Could not allocate redshift_galaxy_colors.\n");
            return 1;
        }
        app_state->cpu_memory_allocated += app_state->redshift_galaxy_count * sizeof(Color);

        i32 redshift_data_init_result = initialize_transforms_redshift_data(app_state);
        if (redshift_data_init_result != 0)
        {
            fprintf(stderr, "ERROR: Could not initialize redshift data matrix transforms.\n");
            return 1;
        }
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

        // Single directional light for the Earth model
        CreateLight(LIGHT_DIRECTIONAL, (Vector3){1000.0f, 1000.0f, 0.0f}, Vector3Zero(), WHITE, app_state->custom_shader);
    }

    // Material - simple solid color, no textures needed for small galaxy spheres
    {
        Material galaxy_material = LoadMaterialDefault();
        galaxy_material.shader = app_state->custom_shader;

        // No textures - solid colors are sufficient for tiny spheres
        galaxy_material.maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
        galaxy_material.maps[MATERIAL_MAP_SPECULAR].value = 0.0f;

        f32 shininess = 1.0f;
        SetShaderValue(galaxy_material.shader, GetShaderLocation(galaxy_material.shader, "shininess"), &shininess, SHADER_UNIFORM_FLOAT);

        app_state->material_instance = galaxy_material;
        app_state->material_instance.shader = app_state->custom_shader;
        app_state->material_instance.maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
    }

    // Redshift material - simple solid color
    {
        Material redshift_mat = LoadMaterialDefault();
        redshift_mat.shader = app_state->custom_shader;

        // Set a warm orange-yellow color to represent the average redshift
        redshift_mat.maps[MATERIAL_MAP_DIFFUSE].color = (Color){255, 180, 80, 255};
        redshift_mat.maps[MATERIAL_MAP_SPECULAR].value = 0.0f;

        app_state->redshift_material = redshift_mat;
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
