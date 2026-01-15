#include "renderer.h"
#include "ui.h"
#include "includes.h"

void renderer_draw_3d(app_state_t *app_state)
{
    BeginMode3D(app_state->main_camera);

    Vector3 earth_pos = {0.0f, 0.0f, 0.0f};
    DrawModel(app_state->earth_model, earth_pos, EARTH_SCALE, WHITE);

    if (app_state->data_to_draw == DRAW_DATA_A || app_state->data_to_draw == DRAW_DATA_ALL)
    {
        const Color GALAXIES_BLUE = {32, 32, 255, 255};
        app_state->material_instance.maps[MATERIAL_MAP_DIFFUSE].color = GALAXIES_BLUE;

        DrawMeshInstanced(
            app_state->sphere_mesh,
            app_state->material_instance,
            app_state->matrix_transforms_a,
            app_state->data_point_count);
    }

    if (app_state->data_to_draw == DRAW_DATA_B || app_state->data_to_draw == DRAW_DATA_ALL)
    {
        const Color GALAXIES_RED = {255, 32, 32, 255};
        app_state->material_instance.maps[MATERIAL_MAP_DIFFUSE].color = GALAXIES_RED;

        DrawMeshInstanced(
            app_state->sphere_mesh,
            app_state->material_instance,
            app_state->matrix_transforms_b,
            app_state->data_point_count);
    }

    if (app_state->data_to_draw == DRAW_DATA_REDSHIFT && app_state->redshift_galaxy_count > 0)
    {
        for (ul i = 0; i < app_state->redshift_galaxy_count; ++i)
        {
            Vector3 pos = {
                app_state->matrix_transforms_redshift[i].m12,
                app_state->matrix_transforms_redshift[i].m13,
                app_state->matrix_transforms_redshift[i].m14};

            f32 size = app_state->matrix_transforms_redshift[i].m0;
            Color base_color = app_state->redshift_galaxy_colors[i];

            Color bright_color = {
                (u8)fmin(255, base_color.r + COLOR_BRIGHTNESS_BOOST),
                (u8)fmin(255, base_color.g + COLOR_BRIGHTNESS_BOOST),
                (u8)fmin(255, base_color.b + COLOR_BRIGHTNESS_BOOST),
                255};

            DrawCube(pos, size, size, size, bright_color);
        }
    }

    EndMode3D();
}

void renderer_draw_frame(app_state_t *app_state)
{
    if (!app_state->data_is_loaded)
    {
        return;
    }

    BeginDrawing();
    const Color clear_color = (Color){4, 4, 8, 255};
    ClearBackground(clear_color);

    renderer_draw_3d(app_state);
    ui_draw(app_state);

    EndDrawing();
}
