#ifndef DATA_TYPES_H
#define DATA_TYPES_H

#include "redefines.h"
#include "raylib_includes.h"

// Window defaults
#define INITIAL_WINDOW_WIDTH 1280
#define INITIAL_WINDOW_HEIGHT 720

//  Camera constants
#define CAMERA_FOV 85.0f
#define CAMERA_MOVE_SPEED 10.0f
#define CAMERA_VERTICAL_SPEED 5.0f
#define CAMERA_MOUSE_SENSITIVITY 0.1f
#define CAMERA_PITCH_LIMIT 89.0f
#define CAMERA_ORBIT_RADIUS 25.0f
#define CAMERA_ORBIT_HEIGHT 50.0f
#define CAMERA_ORBIT_SPEED 0.2f
#define CAMERA_INITIAL_ZOOM (0.8f * PI)
#define CAMERA_INITIAL_YAW 45.80f
#define CAMERA_INITIAL_PITCH 42.12f
#define CAMERA_ZOOM_SPEED 32.0f
#define CAMERA_ZOOM_MIN 0.5f
#define CAMERA_ZOOM_MAX 5.0f
#define CAMERA_SLOW_FACTOR 0.1f

// Free look starting position
#define FREE_LOOK_POS_X 13.632f
#define FREE_LOOK_POS_Y 1.377f
#define FREE_LOOK_POS_Z 9.318f
#define FREE_LOOK_TARGET_X 14.176f
#define FREE_LOOK_TARGET_Y 1.954f
#define FREE_LOOK_TARGET_Z 9.927f
#define FREE_LOOK_DIR_X 0.545f
#define FREE_LOOK_DIR_Y 0.577f
#define FREE_LOOK_DIR_Z 0.609f
#define FREE_LOOK_YAW 48.0f
#define FREE_LOOK_PITCH 35.220f

// Redshift mode camera orbit (farther out, varying pitch)
#define REDSHIFT_ORBIT_RADIUS 64.0f
#define REDSHIFT_ORBIT_SPEED 0.15f
#define REDSHIFT_PITCH_SPEED 0.08f
#define REDSHIFT_PITCH_AMPLITUDE 70.0f

// Rendering constants
#define SPHERE_MESH_RADIUS 0.2f
#define SPHERE_MESH_RINGS 4
#define SPHERE_MESH_SLICES 4
#define CUBE_MESH_SIZE 1.0f
#define EARTH_SCALE 1.0f
#define EARTH_MODEL_SCALE 0.01f
#define COLOR_BRIGHTNESS_BOOST 80

// Course data visualization
#define COURSE_DATA_RADIUS 50.0f
#define COURSE_DATA_SCALE 0.1f
#define ARCMIN_TO_DEGREES 60.0

// Redshift/Hubble constants
#define MIN_VELOCITY_THRESHOLD 500.0
#define VELOCITY_NORMALIZATION_BASE 500.0
#define VELOCITY_NORMALIZATION_RANGE 90000.0
#define RENDER_DISTANCE_MIN 200.0
#define RENDER_DISTANCE_RANGE 600.0
#define RENDER_DISTANCE_MAX (RENDER_DISTANCE_MIN + RENDER_DISTANCE_RANGE)
#define DISTANCE_SIZE_SCALE_MIN 0.5
#define DISTANCE_SIZE_SCALE_MAX 3.0
#define COLOR_VELOCITY_BASE 1000.0
#define COLOR_VELOCITY_RANGE 85000.0
#define COLOR_THRESHOLD_LOW 0.33
#define COLOR_THRESHOLD_MID 0.66
#define COLOR_THRESHOLD_HIGH 0.34

// Magnitude scaling
#define MAGNITUDE_DEFAULT_SCALE 0.6
#define MAGNITUDE_REFERENCE 12.0
#define MAGNITUDE_MAX_VALID 20.0
#define MAGNITUDE_SCALE_MIN 0.3
#define MAGNITUDE_SCALE_MAX 1.2

// FPS display
#define FPS_SMOOTHING_ALPHA 0.1
#define FPS_UPDATE_INTERVAL 0.5f

// Camera clip planes
#define CAMERA_NEAR_PLANE 0.1
#define CAMERA_FAR_PLANE 5000.0

// Shader/Light constants
#define LIGHT_POSITION_X 1000.0f
#define LIGHT_POSITION_Y 1000.0f
#define SHININESS_DEFAULT 1.0f

// Font constants
#define FONT_LOAD_SIZE 128
#define FONT_GLYPH_COUNT 250
#define FONT_SIZE_LARGE 32
#define FONT_SIZE_MEDIUM 24

// UI Panel constants
#define UI_PANEL_MARGIN 8
#define UI_PANEL_PADDING 12
#define UI_INFO_PANEL_WIDTH 340
#define UI_INFO_PANEL_HEIGHT 100
#define UI_MODE_PANEL_WIDTH 260
#define UI_MODE_PANEL_HEIGHT 32
#define UI_LEGEND_PANEL_WIDTH 260
#define UI_LEGEND_PANEL_HEIGHT 110
#define UI_HELP_PANEL_WIDTH 340
#define UI_HELP_PANEL_HEIGHT_NORMAL 170
#define UI_HELP_PANEL_HEIGHT_PAUSED 200
#define UI_DATASET_LEGEND_WIDTH 240
#define UI_DATASET_LEGEND_HEIGHT 130
#define UI_LINE_SPACING 24
#define UI_BOTTOM_HINT_HEIGHT 50
#define UI_COLOR_BAR_HEIGHT 16
#define UI_COLOR_BAR_SEGMENTS 40

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
