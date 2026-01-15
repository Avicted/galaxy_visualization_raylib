#ifndef DATA_TYPES_H
#define DATA_TYPES_H

#include "redefines.h"
#include "raylib_includes.h"

#define INITIAL_WINDOW_WIDTH 1280
#define INITIAL_WINDOW_HEIGHT 720

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
    bool is_paused;
    bool cursor_enabled;
    bool show_help;
    f64 fps_smoothed;
    f64 fps_display;
    f64 fps_update_timer;
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

    Material material_instance;
    Material redshift_material;
    Matrix *matrix_transforms_a;
    Matrix *matrix_transforms_b;

    redshift_galaxy_t *redshift_galaxies;
    Matrix *matrix_transforms_redshift;
    Color *redshift_galaxy_colors;
    ul redshift_galaxy_count;
} app_state_t;

static const ul MAX_DATA_POINTS = 100000UL;
static const ul MAX_REDSHIFT_GALAXIES = 2000UL;

#endif
