#include "renderer.h"
#include "ui.h"
#include "includes.h"
#include "rlgl.h"
#include "raymath.h"

// Draw mesh instanced using pre-uploaded static VBO
// This avoids the per-frame allocation and VBO creation that raylib's DrawMeshInstanced does
void renderer_draw_mesh_instanced_static(Mesh mesh, Material material, u32 instance_vbo, i32 instances)
{
    if (material.shader.id == 0 || material.shader.locs == NULL)
    {
        return;
    }

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

static void
renderer_ensure_render_targets(app_state_t *app_state)
{
    const i32 target_width = app_state->window_width;
    const i32 target_height = app_state->window_height;

    if (target_width <= 0 || target_height <= 0)
    {
        return;
    }

    if (app_state->scene_target.id == 0 ||
        app_state->scene_target.texture.width != target_width ||
        app_state->scene_target.texture.height != target_height)
    {
        if (app_state->scene_target.id != 0)
        {
            UnloadRenderTexture(app_state->scene_target);
        }
        app_state->scene_target = LoadRenderTexture(target_width, target_height);
    }

    if (app_state->glow_target.id == 0 ||
        app_state->glow_target.texture.width != target_width ||
        app_state->glow_target.texture.height != target_height)
    {
        if (app_state->glow_target.id != 0)
        {
            UnloadRenderTexture(app_state->glow_target);
        }
        app_state->glow_target = LoadRenderTexture(target_width, target_height);
    }
}

static bool
renderer_get_course_bounds(app_state_t *app_state, bool use_a, bool use_b,
                           f64 *ra_min, f64 *ra_max, f64 *dec_min, f64 *dec_max)
{
    if (app_state->data_point_count == 0 || (!use_a && !use_b))
    {
        return false;
    }

    f64 min_ra = 1e9;
    f64 max_ra = -1e9;
    f64 min_dec = 1e9;
    f64 max_dec = -1e9;

    for (ul i = 0; i < app_state->data_point_count; ++i)
    {
        if (use_a)
        {
            f64 ra = app_state->data_points_a[i].right_ascension;
            f64 dec = app_state->data_points_a[i].declination;
            min_ra = fmin(min_ra, ra);
            max_ra = fmax(max_ra, ra);
            min_dec = fmin(min_dec, dec);
            max_dec = fmax(max_dec, dec);
        }
        if (use_b)
        {
            f64 ra = app_state->data_points_b[i].right_ascension;
            f64 dec = app_state->data_points_b[i].declination;
            min_ra = fmin(min_ra, ra);
            max_ra = fmax(max_ra, ra);
            min_dec = fmin(min_dec, dec);
            max_dec = fmax(max_dec, dec);
        }
    }

    if (min_ra > max_ra || min_dec > max_dec)
    {
        return false;
    }

    *ra_min = min_ra;
    *ra_max = max_ra;
    *dec_min = min_dec;
    *dec_max = max_dec;
    return true;
}

static void
renderer_draw_course_bounds(app_state_t *app_state)
{
    if (app_state->data_to_draw == DRAW_DATA_REDSHIFT)
    {
        return;
    }

    bool use_a = (app_state->data_to_draw == DRAW_DATA_A || app_state->data_to_draw == DRAW_DATA_ALL);
    bool use_b = (app_state->data_to_draw == DRAW_DATA_B || app_state->data_to_draw == DRAW_DATA_ALL);

    f64 ra_min = 0.0;
    f64 ra_max = 0.0;
    f64 dec_min = 0.0;
    f64 dec_max = 0.0;
    if (!renderer_get_course_bounds(app_state, use_a, use_b, &ra_min, &ra_max, &dec_min, &dec_max))
    {
        return;
    }

    const f64 radius = COURSE_DATA_RADIUS;
    const f64 ra_min_rad = (ra_min / ARCMIN_TO_DEGREES) * DEG2RAD;
    const f64 ra_max_rad = (ra_max / ARCMIN_TO_DEGREES) * DEG2RAD;
    const f64 dec_min_rad = (dec_min / ARCMIN_TO_DEGREES) * DEG2RAD;
    const f64 dec_max_rad = (dec_max / ARCMIN_TO_DEGREES) * DEG2RAD;

    const f64 cos_dec_min = cos(dec_min_rad);
    const f64 cos_dec_max = cos(dec_max_rad);

    Vector3 corner_a = {
        (f32)(radius * cos(ra_min_rad) * cos_dec_min),
        (f32)(radius * sin(dec_min_rad)),
        (f32)(radius * sin(ra_min_rad) * cos_dec_min)};
    Vector3 corner_b = {
        (f32)(radius * cos(ra_max_rad) * cos_dec_min),
        (f32)(radius * sin(dec_min_rad)),
        (f32)(radius * sin(ra_max_rad) * cos_dec_min)};
    Vector3 corner_c = {
        (f32)(radius * cos(ra_min_rad) * cos_dec_max),
        (f32)(radius * sin(dec_max_rad)),
        (f32)(radius * sin(ra_min_rad) * cos_dec_max)};

    const Color line_color = (Color){64, 128, 128, 200};
    DrawLine3D(Vector3Zero(), corner_a, line_color);
    DrawLine3D(Vector3Zero(), corner_b, line_color);
    DrawLine3D(Vector3Zero(), corner_c, line_color);
}

static void
renderer_draw_galaxies_only(app_state_t *app_state)
{
    rlSetClipPlanes(CAMERA_NEAR_PLANE, CAMERA_FAR_PLANE);
    BeginMode3D(app_state->main_camera);

    if (app_state->data_to_draw == DRAW_DATA_A)
    {
        const Color GALAXIES_BLUE = {32, 32, 255, 255};
        app_state->material_instance.maps[MATERIAL_MAP_DIFFUSE].color = GALAXIES_BLUE;

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

        renderer_draw_mesh_instanced_static(
            app_state->sphere_mesh_lowpoly,
            app_state->material_instance,
            app_state->instance_vbo_b,
            app_state->data_point_count);
    }
    else if (app_state->data_to_draw == DRAW_DATA_ALL)
    {
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
        renderer_draw_mesh_instanced_static(
            app_state->sphere_mesh_lowpoly,
            app_state->material_instance,
            app_state->instance_vbo_redshift,
            app_state->redshift_galaxy_count);
    }

    EndMode3D();
}

void renderer_draw_3d(app_state_t *app_state)
{
    rlSetClipPlanes(CAMERA_NEAR_PLANE, CAMERA_FAR_PLANE);
    BeginMode3D(app_state->main_camera);

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

    renderer_draw_course_bounds(app_state);

    // Draw Earth after galaxies with alpha blend to see through it
    // DrawModel uses rlVertex3f internally which is slow
    BeginBlendMode(BLEND_ALPHA);
    const u8 earth_alpha = 190;
    for (i32 i = 0; i < app_state->earth_model.meshCount; i++)
    {
        Material earth_material = app_state->earth_model.materials[app_state->earth_model.meshMaterial[i]];
        earth_material.maps[MATERIAL_MAP_DIFFUSE].color.a = earth_alpha;
        DrawMesh(app_state->earth_model.meshes[i], earth_material, app_state->earth_model.transform);
    }
    EndBlendMode();

    EndMode3D();
}

void renderer_draw_frame(app_state_t *app_state)
{
    if (!app_state->data_is_loaded)
    {
        return;
    }

    const Color clear_color = (Color){4, 4, 8, 255};

    if (app_state->show_start_screen)
    {
        BeginDrawing();
        ClearBackground(clear_color);
        ui_draw_start_screen(app_state);
        EndDrawing();
        return;
    }

    if (app_state->bloom_enabled && app_state->data_to_draw < DRAW_DATA_COUNT)
    {
        renderer_ensure_render_targets(app_state);

        if (app_state->scene_target.id == 0 || app_state->glow_target.id == 0)
        {
            BeginDrawing();
            ClearBackground(clear_color);
            renderer_draw_3d(app_state);
            ui_draw(app_state);
            EndDrawing();
            return;
        }

        BeginTextureMode(app_state->scene_target);
        ClearBackground(clear_color);
        renderer_draw_3d(app_state);
        EndTextureMode();

        BeginTextureMode(app_state->glow_target);
        ClearBackground(BLANK);
        renderer_draw_galaxies_only(app_state);
        EndTextureMode();

        BeginDrawing();
        ClearBackground(clear_color);

        Rectangle src = {0.0f, 0.0f, (f32)app_state->scene_target.texture.width, -(f32)app_state->scene_target.texture.height};
        Rectangle dst = {0.0f, 0.0f, (f32)app_state->window_width, (f32)app_state->window_height};

        DrawTexturePro(app_state->scene_target.texture, src, dst, (Vector2){0.0f, 0.0f}, 0.0f, WHITE);

        BeginBlendMode(BLEND_ADDITIVE);
        BeginShaderMode(app_state->bloom_shader);
        DrawTexturePro(app_state->glow_target.texture, src, dst, (Vector2){0.0f, 0.0f}, 0.0f, WHITE);
        EndShaderMode();
        EndBlendMode();

        ui_draw(app_state);
        EndDrawing();
        return;
    }

    BeginDrawing();
    ClearBackground(clear_color);
    renderer_draw_3d(app_state);
    ui_draw(app_state);
    EndDrawing();
}
