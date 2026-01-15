#include "shaders.h"
#define RLIGHTS_IMPLEMENTATION
#include "../vendor/rlights.h"

#ifdef EMBED_ASSETS
#include "embedded_assets.h"
#endif

i32 shaders_init(app_state_t *app_state)
{
#ifdef EMBED_ASSETS
    app_state->custom_shader = LoadShaderFromMemory(shader_lighting_instancing_vs_data, shader_lighting_fs_data);
#else
    app_state->custom_shader = LoadShader("./shaders/lighting_instancing.vs", "./shaders/lighting.fs");
#endif
    app_state->custom_shader.locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(app_state->custom_shader, "mvp");
    app_state->custom_shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(app_state->custom_shader, "viewPos");
    app_state->custom_shader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocationAttrib(app_state->custom_shader, "instanceTransform");

    {
        i32 ambient_location = GetShaderLocation(app_state->custom_shader, "ambient");
        f32 ambient_value[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        SetShaderValue(app_state->custom_shader, ambient_location, ambient_value, SHADER_UNIFORM_VEC4);

        i32 color_diffuse_loc = GetShaderLocation(app_state->custom_shader, "colorDiffuse");
        f64 diffuse_value[4] = {1.0, 1.0, 1.0, 1.0};
        SetShaderValue(app_state->custom_shader, color_diffuse_loc, &diffuse_value, SHADER_UNIFORM_VEC4);

        CreateLight(LIGHT_DIRECTIONAL, (Vector3){LIGHT_POSITION_X, LIGHT_POSITION_Y, 0.0f}, Vector3Zero(), WHITE, app_state->custom_shader);
    }

    {
        Material galaxy_material = LoadMaterialDefault();
        galaxy_material.shader = app_state->custom_shader;

        galaxy_material.maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
        galaxy_material.maps[MATERIAL_MAP_SPECULAR].value = 0.0f;

        app_state->material_instance = galaxy_material;
        app_state->material_instance.shader = app_state->custom_shader;
        app_state->material_instance.maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
    }

    {
        Material redshift_mat = LoadMaterialDefault();
        redshift_mat.shader = app_state->custom_shader;

        redshift_mat.maps[MATERIAL_MAP_DIFFUSE].color = (Color){255, 180, 80, 255};
        redshift_mat.maps[MATERIAL_MAP_SPECULAR].value = 0.0f;

        app_state->redshift_material = redshift_mat;
    }

    return 0;
}
