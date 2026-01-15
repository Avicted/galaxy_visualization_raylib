#include "renderer.h"
#include "ui.h"
#include "includes.h"
#include "rlgl.h"
#include "raymath.h"

// Draw mesh instanced using pre-uploaded static VBO
// This avoids the per-frame allocation and VBO creation that raylib's DrawMeshInstanced does
void renderer_draw_mesh_instanced_static(Mesh mesh, Material material, u32 instance_vbo, i32 instances)
{
    // Bind shader program
    rlEnableShader(material.shader.id);

    // Upload material color
    if (material.shader.locs[SHADER_LOC_COLOR_DIFFUSE] != -1)
    {
        float values[4] = {
            (float)material.maps[MATERIAL_MAP_DIFFUSE].color.r / 255.0f,
            (float)material.maps[MATERIAL_MAP_DIFFUSE].color.g / 255.0f,
            (float)material.maps[MATERIAL_MAP_DIFFUSE].color.b / 255.0f,
            (float)material.maps[MATERIAL_MAP_DIFFUSE].color.a / 255.0f};
        rlSetUniform(material.shader.locs[SHADER_LOC_COLOR_DIFFUSE], values, SHADER_UNIFORM_VEC4, 1);
    }

    // Get current matrices
    Matrix matModel = MatrixIdentity();
    Matrix matView = rlGetMatrixModelview();
    Matrix matProjection = rlGetMatrixProjection();
    Matrix matModelView = MatrixMultiply(matModel, matView);
    Matrix matModelViewProjection = MatrixMultiply(matModelView, matProjection);

    // Upload matrices
    if (material.shader.locs[SHADER_LOC_MATRIX_MVP] != -1)
    {
        rlSetUniformMatrix(material.shader.locs[SHADER_LOC_MATRIX_MVP], matModelViewProjection);
    }
    if (material.shader.locs[SHADER_LOC_MATRIX_VIEW] != -1)
    {
        rlSetUniformMatrix(material.shader.locs[SHADER_LOC_MATRIX_VIEW], matView);
    }
    if (material.shader.locs[SHADER_LOC_MATRIX_PROJECTION] != -1)
    {
        rlSetUniformMatrix(material.shader.locs[SHADER_LOC_MATRIX_PROJECTION], matProjection);
    }
    if (material.shader.locs[SHADER_LOC_MATRIX_MODEL] != -1)
    {
        rlSetUniformMatrix(material.shader.locs[SHADER_LOC_MATRIX_MODEL], matModel);
    }
    if (material.shader.locs[SHADER_LOC_MATRIX_NORMAL] != -1)
    {
        rlSetUniformMatrix(material.shader.locs[SHADER_LOC_MATRIX_NORMAL], MatrixTranspose(MatrixInvert(matModel)));
    }

    // Bind diffuse texture
    rlActiveTextureSlot(0);
    rlEnableTexture(material.maps[MATERIAL_MAP_DIFFUSE].texture.id);

    // Enable mesh VAO
    rlEnableVertexArray(mesh.vaoId);

    // Bind our pre-uploaded static instance VBO
    rlEnableVertexBuffer(instance_vbo);

    // Set up instance transform attribute (mat4 = 4 vec4 attributes)
    // The location is stored in SHADER_LOC_MATRIX_MODEL (see shaders.c initialization)
    i32 instance_loc = material.shader.locs[SHADER_LOC_MATRIX_MODEL];
    if (instance_loc != -1)
    {
        for (u32 i = 0; i < 4; i++)
        {
            rlEnableVertexAttribute(instance_loc + i);
            rlSetVertexAttribute(instance_loc + i, 4, RL_FLOAT, 0, sizeof(Matrix), i * sizeof(Vector4));
            rlSetVertexAttributeDivisor(instance_loc + i, 1);
        }
    }

    // Draw instanced
    if (mesh.indices != NULL)
    {
        rlDrawVertexArrayElementsInstanced(0, mesh.triangleCount * 3, 0, instances);
    }
    else
    {
        rlDrawVertexArrayInstanced(0, mesh.vertexCount, instances);
    }

    // Cleanup
    rlDisableVertexArray();
    rlDisableVertexBuffer();
    rlDisableTexture();
    rlDisableShader();
}

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

        // Use static VBO - no per-frame allocation!
        renderer_draw_mesh_instanced_static(
            app_state->sphere_mesh_lowpoly,
            app_state->material_instance,
            app_state->instance_vbo_a,
            app_state->data_point_count);
    }
    else if (app_state->data_to_draw == DRAW_DATA_B)
    {
        const Color GALAXIES_RED = {255, 32, 32, 255};
        app_state->material_instance.maps[MATERIAL_MAP_DIFFUSE].color = GALAXIES_RED;

        // Use static VBO - no per-frame allocation!
        renderer_draw_mesh_instanced_static(
            app_state->sphere_mesh_lowpoly,
            app_state->material_instance,
            app_state->instance_vbo_b,
            app_state->data_point_count);
    }
    else if (app_state->data_to_draw == DRAW_DATA_ALL)
    {
        // Two draw calls to preserve distinct colors for each dataset
        const Color GALAXIES_BLUE = {32, 32, 255, 255};
        app_state->material_instance.maps[MATERIAL_MAP_DIFFUSE].color = GALAXIES_BLUE;

        renderer_draw_mesh_instanced_static(
            app_state->sphere_mesh_lowpoly,
            app_state->material_instance,
            app_state->instance_vbo_a,
            app_state->data_point_count);

        const Color GALAXIES_RED = {255, 32, 32, 255};
        app_state->material_instance.maps[MATERIAL_MAP_DIFFUSE].color = GALAXIES_RED;

        renderer_draw_mesh_instanced_static(
            app_state->sphere_mesh_lowpoly,
            app_state->material_instance,
            app_state->instance_vbo_b,
            app_state->data_point_count);
    }
    else if (app_state->data_to_draw == DRAW_DATA_REDSHIFT && app_state->redshift_galaxy_count > 0)
    {
        // Use static VBO for redshift data too
        renderer_draw_mesh_instanced_static(
            app_state->sphere_mesh_lowpoly,
            app_state->material_instance,
            app_state->instance_vbo_redshift,
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
