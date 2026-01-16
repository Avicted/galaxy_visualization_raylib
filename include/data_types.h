#ifndef DATA_TYPES_H
#define DATA_TYPES_H

#include "redefines.h"
#include "raylib_includes.h"
#include "constants.h"

typedef struct
{
    f64 right_ascension;
    f64 declination;
} arcmin_data_t;

typedef struct
{
    char name[16];
    f64 right_ascension;
    f64 declination;
    f64 helio_velocity;
    f64 b_magnitude;
} redshift_galaxy_t;

typedef enum
{
    DRAW_DATA_A,
    DRAW_DATA_B,
    DRAW_DATA_ALL,
    DRAW_DATA_REDSHIFT,
    DRAW_DATA_COUNT,
} draw_data_t;

typedef struct
{
    arcmin_data_t *data_points_a;
    arcmin_data_t *data_points_b;
    ul data_point_count;

    draw_data_t data_to_draw;

    u64 cpu_memory_allocated;
    bool debug;
    bool data_is_loaded;
    bool show_start_screen;
    bool is_paused;
    bool cursor_enabled;
    bool show_help;
    f64 fps_smoothed;
    f64 fps_display;
    f64 fps_update_timer;
    Font main_font;
    Font start_screen_font;
    i32 window_width;
    i32 window_height;

    Camera3D main_camera;
    f64 camera_zoom;
    f64 camera_yaw;
    f64 camera_pitch;
    Vector3 camera_direction;
    Vector3 course_center_a;
    Vector3 course_center_b;
    Vector3 course_center_all;
    f32 course_radius_a;
    f32 course_radius_b;
    f32 course_radius_all;

    Shader custom_shader;
    Shader bloom_shader;
    RenderTexture2D scene_target;
    RenderTexture2D glow_target;
    Mesh sphere_mesh_lowpoly;
    Model earth_model;

    Material material_instance;
    Material redshift_material;
    Matrix *matrix_transforms_a;
    Matrix *matrix_transforms_b;

    // Static GPU instance buffers (uploaded once, reused every frame)
    u32 instance_vbo_a;
    u32 instance_vbo_b;
    u32 instance_vbo_redshift;

    redshift_galaxy_t *redshift_galaxies;
    Matrix *matrix_transforms_redshift;
    Color *redshift_galaxy_colors;
    ul redshift_galaxy_count;
} app_state_t;

#endif
