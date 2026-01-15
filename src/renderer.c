#include "renderer.h"
#include "ui.h"
#include "includes.h"
#include "rlgl.h"

void renderer_draw_3d(app_state_t *app_state)
{
    rlSetClipPlanes(CAMERA_NEAR_PLANE, CAMERA_FAR_PLANE);
    BeginMode3D(app_state->main_camera);

    Vector3 earth_pos = {0.0f, 0.0f, 0.0f};
    DrawModel(app_state->earth_model, earth_pos, EARTH_SCALE, WHITE);

    if (app_state->data_to_draw == DRAW_DATA_A)
    {
        const Color GALAXIES_BLUE = {32, 32, 255, 255};
        app_state->material_instance.maps[MATERIAL_MAP_DIFFUSE].color = GALAXIES_BLUE;

        // Use low-poly mesh for non-redshifted galaxies (performance)
        DrawMeshInstanced(
            app_state->sphere_mesh_lowpoly,
            app_state->material_instance,
            app_state->matrix_transforms_a,
            app_state->data_point_count);
    }
    else if (app_state->data_to_draw == DRAW_DATA_B)
    {
        const Color GALAXIES_RED = {255, 32, 32, 255};
        app_state->material_instance.maps[MATERIAL_MAP_DIFFUSE].color = GALAXIES_RED;

        // Use low-poly mesh for non-redshifted galaxies (performance)
        DrawMeshInstanced(
            app_state->sphere_mesh_lowpoly,
            app_state->material_instance,
            app_state->matrix_transforms_b,
            app_state->data_point_count);
    }
    else if (app_state->data_to_draw == DRAW_DATA_ALL)
    {
        // Two draw calls to preserve distinct colors for each dataset
        const Color GALAXIES_BLUE = {32, 32, 255, 255};
        app_state->material_instance.maps[MATERIAL_MAP_DIFFUSE].color = GALAXIES_BLUE;

        DrawMeshInstanced(
            app_state->sphere_mesh_lowpoly,
            app_state->material_instance,
            app_state->matrix_transforms_a,
            app_state->data_point_count);

        const Color GALAXIES_RED = {255, 32, 32, 255};
        app_state->material_instance.maps[MATERIAL_MAP_DIFFUSE].color = GALAXIES_RED;

        DrawMeshInstanced(
            app_state->sphere_mesh_lowpoly,
            app_state->material_instance,
            app_state->matrix_transforms_b,
            app_state->data_point_count);
    }
    else if (app_state->data_to_draw == DRAW_DATA_REDSHIFT && app_state->redshift_galaxy_count > 0)
    {
        // Use standard instanced rendering - colors are encoded in matrix slots
        DrawMeshInstanced(
            app_state->sphere_mesh_lowpoly,
            app_state->material_instance,
            app_state->matrix_transforms_redshift,
            app_state->redshift_galaxy_count);
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
