#include "transforms.h"
#include "includes.h"
#include "macros.h"
#include "rlgl.h"

static Vector3
transforms_course_position(f64 right_ascension, f64 declination)
{
    const f64 right_ascension_rad = (right_ascension / ARCMIN_TO_DEGREES) * DEG2RAD;
    const f64 declination_rad = (declination / ARCMIN_TO_DEGREES) * DEG2RAD;
    const f64 radius = COURSE_DATA_RADIUS;
    const f64 cos_dec = cos(declination_rad);

    Vector3 pos;
    pos.x = (f32)(radius * cos(right_ascension_rad) * cos_dec);
    pos.y = (f32)(radius * sin(declination_rad));
    pos.z = (f32)(radius * sin(right_ascension_rad) * cos_dec);
    return pos;
}

static void
transforms_course_center_and_radius(f64 ra_min, f64 ra_max, f64 dec_min, f64 dec_max,
                                    Vector3 *out_center, f32 *out_radius)
{
    const f64 ra_mid = 0.5 * (ra_min + ra_max);
    const f64 dec_mid = 0.5 * (dec_min + dec_max);

    Vector3 center = transforms_course_position(ra_mid, dec_mid);

    Vector3 corner_a = transforms_course_position(ra_min, dec_min);
    Vector3 corner_b = transforms_course_position(ra_max, dec_min);
    Vector3 corner_c = transforms_course_position(ra_min, dec_max);
    Vector3 corner_d = transforms_course_position(ra_max, dec_max);

    f32 max_dist_sq = 0.0f;
    Vector3 corners[4] = {corner_a, corner_b, corner_c, corner_d};
    for (i32 i = 0; i < 4; ++i)
    {
        Vector3 d = Vector3Subtract(corners[i], center);
        f32 dist_sq = d.x * d.x + d.y * d.y + d.z * d.z;
        max_dist_sq = fmaxf(max_dist_sq, dist_sq);
    }

    *out_center = center;
    *out_radius = sqrtf(max_dist_sq);
}

f64 transforms_distance_from_velocity(f64 velocity_km_s)
{
    if (velocity_km_s <= 0.0)
    {
        return REDSHIFT_RENDER_DISTANCE_MIN;
    }

    f64 normalized = (velocity_km_s - REDSHIFT_VELOCITY_NORMALIZATION_BASE) / REDSHIFT_VELOCITY_NORMALIZATION_RANGE;
    normalized = fmax(0.0, fmin(normalized, 1.0));

    f64 render_distance = REDSHIFT_RENDER_DISTANCE_MIN + sqrt(normalized) * REDSHIFT_RENDER_DISTANCE_RANGE;
    return render_distance;
}

Color transforms_color_from_velocity(f64 velocity_km_s)
{
    f64 t = (velocity_km_s - REDSHIFT_COLOR_VELOCITY_BASE) / REDSHIFT_COLOR_VELOCITY_RANGE;
    t = fmax(0.0, fmin(t, 1.0));

    Color color;

    if (t < REDSHIFT_COLOR_THRESHOLD_LOW)
    {
        f64 s = t / REDSHIFT_COLOR_THRESHOLD_LOW;
        color.r = (u8)(s * 255.0);
        color.g = (u8)(200.0 + s * 55.0);
        color.b = (u8)(255.0 - s * 175.0);
        color.a = 255;
    }
    else if (t < REDSHIFT_COLOR_THRESHOLD_MID)
    {
        f64 s = (t - REDSHIFT_COLOR_THRESHOLD_LOW) / REDSHIFT_COLOR_THRESHOLD_LOW;
        color.r = 255;
        color.g = (u8)(230 - s * 100);
        color.b = (u8)(55 - s * 35);
        color.a = 255;
    }
    else
    {
        f64 s = (t - REDSHIFT_COLOR_THRESHOLD_MID) / REDSHIFT_COLOR_THRESHOLD_HIGH;
        color.r = (u8)(255 - s * 55);
        color.g = (u8)(130 - s * 90);
        color.b = (u8)(20 - s * 10);
        color.a = 255;
    }

    return color;
}

i32 transforms_init_course_data(app_state_t *app_state)
{
    f64 min_ra_a = 1e18;
    f64 max_ra_a = -1e18;
    f64 min_dec_a = 1e18;
    f64 max_dec_a = -1e18;
    f64 min_ra_b = 1e18;
    f64 max_ra_b = -1e18;
    f64 min_dec_b = 1e18;
    f64 max_dec_b = -1e18;

    for (ul i = 0; i < app_state->data_point_count; ++i)
    {
        min_ra_a = fmin(min_ra_a, app_state->data_points_a[i].right_ascension);
        max_ra_a = fmax(max_ra_a, app_state->data_points_a[i].right_ascension);
        min_dec_a = fmin(min_dec_a, app_state->data_points_a[i].declination);
        max_dec_a = fmax(max_dec_a, app_state->data_points_a[i].declination);

        min_ra_b = fmin(min_ra_b, app_state->data_points_b[i].right_ascension);
        max_ra_b = fmax(max_ra_b, app_state->data_points_b[i].right_ascension);
        min_dec_b = fmin(min_dec_b, app_state->data_points_b[i].declination);
        max_dec_b = fmax(max_dec_b, app_state->data_points_b[i].declination);

        // Dataset A transforms
        {
            const f64 right_ascension_rad = (app_state->data_points_a[i].right_ascension / ARCMIN_TO_DEGREES) * DEG2RAD;
            const f64 declination_rad = (app_state->data_points_a[i].declination / ARCMIN_TO_DEGREES) * DEG2RAD;
            const f64 radius = COURSE_DATA_RADIUS;
            const f64 cos_dec = cos(declination_rad);
            const f64 x = radius * cos(right_ascension_rad) * cos_dec;
            const f64 y = radius * sin(declination_rad);
            const f64 z = radius * sin(right_ascension_rad) * cos_dec;

            // Build transform directly instead of multiple matrix multiplies
            Matrix transform = MatrixScale(COURSE_DATA_SCALE, COURSE_DATA_SCALE, COURSE_DATA_SCALE);
            transform.m12 = (f32)x;
            transform.m13 = (f32)y;
            transform.m14 = (f32)z;

            app_state->matrix_transforms_a[i] = transform;
        }

        // Dataset B transforms
        {
            const f64 right_ascension_rad = (app_state->data_points_b[i].right_ascension / ARCMIN_TO_DEGREES) * DEG2RAD;
            const f64 declination_rad = (app_state->data_points_b[i].declination / ARCMIN_TO_DEGREES) * DEG2RAD;
            const f64 radius = COURSE_DATA_RADIUS;
            const f64 cos_dec = cos(declination_rad);
            const f64 x = radius * cos(right_ascension_rad) * cos_dec;
            const f64 y = radius * sin(declination_rad);
            const f64 z = radius * sin(right_ascension_rad) * cos_dec;

            // Build transform directly instead of multiple matrix multiplies
            Matrix transform = MatrixScale(COURSE_DATA_SCALE, COURSE_DATA_SCALE, COURSE_DATA_SCALE);
            transform.m12 = (f32)x;
            transform.m13 = (f32)y;
            transform.m14 = (f32)z;

            app_state->matrix_transforms_b[i] = transform;
        }
    }

    if (app_state->data_point_count > 0)
    {
        transforms_course_center_and_radius(min_ra_a, max_ra_a, min_dec_a, max_dec_a,
                                            &app_state->course_center_a, &app_state->course_radius_a);
        transforms_course_center_and_radius(min_ra_b, max_ra_b, min_dec_b, max_dec_b,
                                            &app_state->course_center_b, &app_state->course_radius_b);

        f64 min_ra_all = fmin(min_ra_a, min_ra_b);
        f64 max_ra_all = fmax(max_ra_a, max_ra_b);
        f64 min_dec_all = fmin(min_dec_a, min_dec_b);
        f64 max_dec_all = fmax(max_dec_a, max_dec_b);
        transforms_course_center_and_radius(min_ra_all, max_ra_all, min_dec_all, max_dec_all,
                                            &app_state->course_center_all, &app_state->course_radius_all);
    }

    return 0;
}

i32 transforms_init_redshift_data(app_state_t *app_state)
{
    for (ul i = 0; i < app_state->redshift_galaxy_count; ++i)
    {
        redshift_galaxy_t *galaxy = &app_state->redshift_galaxies[i];

        const f64 right_ascension_rad = galaxy->right_ascension * DEG2RAD;
        const f64 declination_rad = galaxy->declination * DEG2RAD;
        const f64 radius = transforms_distance_from_velocity(galaxy->helio_velocity);

        const f64 x = radius * cos(declination_rad) * cos(right_ascension_rad);
        const f64 y = radius * sin(declination_rad);
        const f64 z = radius * cos(declination_rad) * sin(right_ascension_rad);

        app_state->matrix_transforms_redshift[i] = MatrixIdentity();

        f64 magnitude_scale = MAGNITUDE_DEFAULT_SCALE;
        if (galaxy->b_magnitude > 0.0 && galaxy->b_magnitude < MAGNITUDE_MAX_VALID)
        {
            magnitude_scale = 1.0 / (galaxy->b_magnitude / MAGNITUDE_REFERENCE);
            magnitude_scale = fmax(MAGNITUDE_SCALE_MIN, fmin(magnitude_scale, MAGNITUDE_SCALE_MAX));
        }

        // Scale size based on distance so farther galaxies remain visible
        f64 distance_t = (radius - REDSHIFT_RENDER_DISTANCE_MIN) / REDSHIFT_RENDER_DISTANCE_RANGE;
        distance_t = fmax(0.0, fmin(distance_t, 1.0));
        f64 distance_scale = REDSHIFT_DISTANCE_SIZE_SCALE_MIN + distance_t * (REDSHIFT_DISTANCE_SIZE_SCALE_MAX - REDSHIFT_DISTANCE_SIZE_SCALE_MIN);
        f64 final_scale = magnitude_scale * distance_scale;

        app_state->matrix_transforms_redshift[i] = MatrixMultiply(
            app_state->matrix_transforms_redshift[i],
            MatrixScale((f32)final_scale, (f32)final_scale, (f32)final_scale));
        app_state->matrix_transforms_redshift[i] = MatrixMultiply(
            app_state->matrix_transforms_redshift[i],
            MatrixTranslate((f32)x, (f32)y, (f32)z));

        Color color = transforms_color_from_velocity(galaxy->helio_velocity);
        // Apply brightness boost
        color.r = (u8)fmin(255, color.r + COLOR_BRIGHTNESS_BOOST);
        color.g = (u8)fmin(255, color.g + COLOR_BRIGHTNESS_BOOST);
        color.b = (u8)fmin(255, color.b + COLOR_BRIGHTNESS_BOOST);
        app_state->redshift_galaxy_colors[i] = color;

        // Encode color into matrix for GPU instancing
        // Use m1, m2, m4 - these are off-diagonal rotation elements (zero in scale+translate)
        // Raylib Matrix is row-major: m1=[row0,col1], m2=[row0,col2], m4=[row1,col0]
        app_state->matrix_transforms_redshift[i].m1 = (f32)color.r / 255.0f;
        app_state->matrix_transforms_redshift[i].m2 = (f32)color.g / 255.0f;
        app_state->matrix_transforms_redshift[i].m4 = (f32)color.b / 255.0f;
    }

    return 0;
}

i32 transforms_upload_to_gpu(app_state_t *app_state)
{
    app_state->matrix_transforms_a = (Matrix *)calloc(MAX_DATA_POINTS, sizeof(Matrix));
    if (app_state->matrix_transforms_a == NULL)
    {
        fprintf(stderr, "[ERROR] Alloc failed: matrix_transforms_a\n");
        return 1;
    }
    app_state->cpu_memory_allocated += MAX_DATA_POINTS * sizeof(Matrix);

    app_state->matrix_transforms_b = (Matrix *)calloc(MAX_DATA_POINTS, sizeof(Matrix));
    if (app_state->matrix_transforms_b == NULL)
    {
        fprintf(stderr, "[ERROR] Alloc failed: matrix_transforms_b\n");
        return 1;
    }
    app_state->cpu_memory_allocated += MAX_DATA_POINTS * sizeof(Matrix);

    i32 course_data_init_result = transforms_init_course_data(app_state);
    if (course_data_init_result != 0)
    {
        fprintf(stderr, "[ERROR] Transform init failed: course data\n");
        return 1;
    }

    if (app_state->redshift_galaxy_count > 0)
    {
        app_state->matrix_transforms_redshift = (Matrix *)calloc(app_state->redshift_galaxy_count, sizeof(Matrix));
        if (app_state->matrix_transforms_redshift == NULL)
        {
            fprintf(stderr, "[ERROR] Alloc failed: matrix_transforms_redshift\n");
            return 1;
        }
        app_state->cpu_memory_allocated += app_state->redshift_galaxy_count * sizeof(Matrix);

        app_state->redshift_galaxy_colors = (Color *)calloc(app_state->redshift_galaxy_count, sizeof(Color));
        if (app_state->redshift_galaxy_colors == NULL)
        {
            fprintf(stderr, "[ERROR] Alloc failed: redshift_galaxy_colors\n");
            return 1;
        }
        app_state->cpu_memory_allocated += app_state->redshift_galaxy_count * sizeof(Color);

        i32 redshift_data_init_result = transforms_init_redshift_data(app_state);
        if (redshift_data_init_result != 0)
        {
            fprintf(stderr, "[ERROR] Transform init failed: redshift data\n");
            return 1;
        }
    }

    return 0;
}

// Upload instance transforms to GPU as static VBOs (called after OpenGL context is ready)
i32 transforms_upload_instance_vbos(app_state_t *app_state)
{
    // Convert matrices to float16 format (column-major for OpenGL)
    // This is what raylib does every frame - we do it once
    float16 *instance_data_a = (float16 *)RL_MALLOC(app_state->data_point_count * sizeof(float16));
    float16 *instance_data_b = (float16 *)RL_MALLOC(app_state->data_point_count * sizeof(float16));

    if (!instance_data_a || !instance_data_b)
    {
        fprintf(stderr, "[ERROR] Failed to allocate instance data buffers\n");
        return 1;
    }

    for (ul i = 0; i < app_state->data_point_count; i++)
    {
        instance_data_a[i] = MatrixToFloatV(app_state->matrix_transforms_a[i]);
        instance_data_b[i] = MatrixToFloatV(app_state->matrix_transforms_b[i]);
    }

    // Upload to GPU as static VBOs
    app_state->instance_vbo_a = rlLoadVertexBuffer(instance_data_a,
                                                   app_state->data_point_count * sizeof(float16), false);
    app_state->instance_vbo_b = rlLoadVertexBuffer(instance_data_b,
                                                   app_state->data_point_count * sizeof(float16), false);

    RL_FREE(instance_data_a);
    RL_FREE(instance_data_b);

    printf("[INFO]  Static instance VBOs uploaded: A=%u, B=%u\n",
           app_state->instance_vbo_a, app_state->instance_vbo_b);

    // Upload redshift data if available
    if (app_state->redshift_galaxy_count > 0)
    {
        float16 *instance_data_redshift = (float16 *)RL_MALLOC(app_state->redshift_galaxy_count * sizeof(float16));
        if (!instance_data_redshift)
        {
            fprintf(stderr, "[ERROR] Failed to allocate redshift instance data\n");
            return 1;
        }

        for (ul i = 0; i < app_state->redshift_galaxy_count; i++)
        {
            instance_data_redshift[i] = MatrixToFloatV(app_state->matrix_transforms_redshift[i]);
        }

        app_state->instance_vbo_redshift = rlLoadVertexBuffer(instance_data_redshift,
                                                              app_state->redshift_galaxy_count * sizeof(float16), false);

        RL_FREE(instance_data_redshift);
        printf("[INFO]  Redshift instance VBO uploaded: %u\n", app_state->instance_vbo_redshift);
    }

    return 0;
}

void transforms_cleanup_instance_vbos(app_state_t *app_state)
{
    if (app_state->instance_vbo_a)
    {
        rlUnloadVertexBuffer(app_state->instance_vbo_a);
        app_state->instance_vbo_a = 0;
    }
    if (app_state->instance_vbo_b)
    {
        rlUnloadVertexBuffer(app_state->instance_vbo_b);
        app_state->instance_vbo_b = 0;
    }
    if (app_state->instance_vbo_redshift)
    {
        rlUnloadVertexBuffer(app_state->instance_vbo_redshift);
        app_state->instance_vbo_redshift = 0;
    }
}
