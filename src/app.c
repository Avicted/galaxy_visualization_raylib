#include "app.h"
#include "camera.h"
#include "data_loader.h"
#include "renderer.h"
#include "shaders.h"
#include "transforms.h"
#include "utils.h"
#include "includes.h"
#include "macros.h"
#include "asset_io.h"

#ifndef APP_VERSION
#define APP_VERSION "0.0.0"
#endif

internal i32
app_init_platform(app_state_t *app_state)
{
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    const char *window_title = TextFormat("Galaxy Visualization v%s", APP_VERSION);
    InitWindow(app_state->window_width, app_state->window_height, window_title);

    {
        Image icon = asset_io_load_image(ASSET_ICON_APP, ".png");
        if (icon.data != NULL)
        {
            SetWindowIcon(icon);
            UnloadImage(icon);
        }
        else
        {
            fprintf(stderr, "[WARN]  Failed to load window icon\n");
        }
    }

    return 0;
}

internal void
app_update(app_state_t *app_state, f64 dt)
{
    camera_handle_resize(app_state);

    if (app_state->show_start_screen)
    {
        if (IsKeyPressed(KEY_ESCAPE))
        {
            CloseWindow();
        }

        if (IsKeyPressed(KEY_SPACE))
        {
            app_state->show_start_screen = false;
        }

        return;
    }

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
        printf("[DEBUG] Paused: %s\n", app_state->is_paused ? "yes" : "no");
    }

    if (IsKeyPressed(KEY_H))
    {
        app_state->show_help = !app_state->show_help;
    }

    camera_update(app_state, dt);

    f64 scroll = GetMouseWheelMove();
    if (scroll != 0.0f)
    {
        const f64 zoom_change = -CAMERA_ZOOM_SPEED;
        app_state->camera_zoom = Clamp(app_state->camera_zoom + scroll * zoom_change * dt, CAMERA_ZOOM_MIN, CAMERA_ZOOM_MAX);
    }

    if (dt > 0.0)
    {
        f64 fps_inst = 1.0 / dt;
        if (app_state->fps_smoothed <= 0.0)
        {
            app_state->fps_smoothed = fps_inst;
            app_state->fps_display = fps_inst;
        }
        else
        {
            const f64 alpha = FPS_SMOOTHING_ALPHA;
            app_state->fps_smoothed = app_state->fps_smoothed * (1.0 - alpha) + fps_inst * alpha;
        }

        app_state->fps_update_timer += dt;
        if (app_state->fps_update_timer >= FPS_UPDATE_INTERVAL)
        {
            app_state->fps_display = app_state->fps_smoothed;
            app_state->fps_update_timer = 0.0;
        }
    }
}

app_state_t *
app_create(void)
{
    app_state_t *app_state = (app_state_t *)calloc(1, sizeof(app_state_t));
    if (app_state == NULL)
    {
        fprintf(stderr, "[ERROR] Failed to allocate app_state\n");
        return NULL;
    }
    return app_state;
}

void app_parse_args(app_state_t *app_state, i32 argc, char **argv)
{
    if (argc == 1)
    {
        printf("[INFO]  Working directory: %s\n", GetWorkingDirectory());
        return;
    }

    for (i32 i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "GALAXY_DEBUG") == 0)
        {
            printf("[INFO]  Debug mode enabled\n");
            app_state->debug = true;
        }
    }
}

i32 app_init(app_state_t *app_state)
{
    bool prev_debug = app_state->debug;

    memset(app_state, 0, sizeof(app_state_t));

    app_state->window_width = INITIAL_WINDOW_WIDTH;
    app_state->window_height = INITIAL_WINDOW_HEIGHT;
    app_state->debug = prev_debug;
    app_state->cursor_enabled = true;
    app_state->show_start_screen = true;
    app_state->show_help = false; // Hidden by default for better performance

    app_state->main_camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    app_state->main_camera.fovy = CAMERA_FOV;
    app_state->main_camera.projection = CAMERA_PERSPECTIVE;

    app_state->camera_zoom = CAMERA_INITIAL_ZOOM;
    app_state->camera_yaw = CAMERA_INITIAL_YAW;
    app_state->camera_pitch = CAMERA_INITIAL_PITCH;

    app_state->data_to_draw = DRAW_DATA_ALL;

    if (data_loader_load_all(app_state) != 0)
    {
        fprintf(stderr, "[ERROR] Failed to read input data\n");
        return 1;
    }

    printf("[INFO]  Galaxy Visualization v%s initialized\n", APP_VERSION);

    if (transforms_upload_to_gpu(app_state) != 0)
    {
        fprintf(stderr, "[ERROR] Failed to upload transforms to GPU\n");
        return 1;
    }

    if (app_init_platform(app_state) != 0)
    {
        fprintf(stderr, "[ERROR] Failed to initialize platform\n");
        return 1;
    }

    {
        app_state->main_font = asset_io_load_font(ASSET_FONT_PERFECT_DOS, FONT_LOAD_SIZE, NULL, FONT_GLYPH_COUNT);
        if (app_state->main_font.texture.id == 0)
        {
            fprintf(stderr, "[ERROR] Failed to load font data\n");
        }
    }

    {
        app_state->start_screen_font = asset_io_load_font(ASSET_FONT_ABUGET, FONT_LOAD_SIZE * 2, NULL, FONT_GLYPH_COUNT * 3);
        if (app_state->start_screen_font.texture.id == 0)
        {
            fprintf(stderr, "[WARN]  Failed to load start screen font, using default\n");
            app_state->start_screen_font = app_state->main_font;
        }
    }

    app_state->sphere_mesh_lowpoly = GenMeshSphere(SPHERE_MESH_RADIUS, SPHERE_LOWPOLY_RINGS, SPHERE_LOWPOLY_SLICES);

    if (shaders_init(app_state) != 0)
    {
        fprintf(stderr, "[ERROR] Failed to initialize shaders\n");
        return 1;
    }

    if (transforms_upload_instance_vbos(app_state) != 0)
    {
        fprintf(stderr, "[ERROR] Failed to upload instance VBOs to GPU\n");
        return 1;
    }

    print_memory_usage(app_state);

    app_state->earth_model = asset_io_load_model(ASSET_MODEL_EARTH);
    Matrix earth_scale_matrix = MatrixScale(EARTH_MODEL_SCALE, EARTH_MODEL_SCALE, EARTH_MODEL_SCALE);
    app_state->earth_model.transform = MatrixMultiply(app_state->earth_model.transform, earth_scale_matrix);

    return 0;
}

void app_run(app_state_t *app_state)
{
    while (!WindowShouldClose())
    {
        const f64 dt = GetFrameTime();
        app_update(app_state, dt);
        renderer_draw_frame(app_state);
    }
}

void app_cleanup(app_state_t *app_state)
{
    if (!app_state)
    {
        return;
    }

    transforms_cleanup_instance_vbos(app_state);

    if (app_state->earth_model.meshCount > 0)
    {
        UnloadModel(app_state->earth_model);
    }
    if (app_state->sphere_mesh_lowpoly.vboId)
    {
        UnloadMesh(app_state->sphere_mesh_lowpoly);
    }
    if (app_state->material_instance.shader.id)
    {
        UnloadShader(app_state->material_instance.shader);
    }

    if (app_state->main_font.texture.id)
    {
        UnloadFont(app_state->main_font);
    }

    if (app_state->start_screen_font.texture.id && app_state->start_screen_font.texture.id != app_state->main_font.texture.id)
    {
        UnloadFont(app_state->start_screen_font);
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

        if (app_state->show_start_screen)
        {
            if (IsKeyPressed(KEY_ESCAPE))
            {
                CloseWindow();
            }

            if (IsKeyPressed(KEY_SPACE))
            {
                app_state->show_start_screen = false;
            }

            return;
        }
    {
        free(app_state->redshift_galaxy_colors);
        app_state->cpu_memory_allocated -= app_state->redshift_galaxy_count * sizeof(Color);
    }

    printf("[INFO]  Cleanup complete\n");
    print_memory_usage(app_state);

    ASSERT(app_state->cpu_memory_allocated == 0);

    CloseWindow();

    free(app_state);
}
