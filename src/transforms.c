#include "transforms.h"
#include "includes.h"
#include "macros.h"

f64 transforms_distance_from_velocity(f64 velocity_km_s)
{
    if (velocity_km_s <= 0.0)
    {
        return RENDER_DISTANCE_MIN;
    }

    f64 normalized = (velocity_km_s - VELOCITY_NORMALIZATION_BASE) / VELOCITY_NORMALIZATION_RANGE;
    normalized = fmax(0.0, fmin(normalized, 1.0));

    f64 render_distance = RENDER_DISTANCE_MIN + sqrt(normalized) * RENDER_DISTANCE_RANGE;
    return render_distance;
}

Color transforms_color_from_velocity(f64 velocity_km_s)
{
    f64 t = (velocity_km_s - COLOR_VELOCITY_BASE) / COLOR_VELOCITY_RANGE;
    t = fmax(0.0, fmin(t, 1.0));

    Color color;

    if (t < COLOR_THRESHOLD_LOW)
    {
        f64 s = t / COLOR_THRESHOLD_LOW;
        color.r = (u8)(s * 255.0);
        color.g = (u8)(200.0 + s * 55.0);
        color.b = (u8)(255.0 - s * 175.0);
        color.a = 255;
    }
    else if (t < COLOR_THRESHOLD_MID)
    {
        f64 s = (t - COLOR_THRESHOLD_LOW) / COLOR_THRESHOLD_LOW;
        color.r = 255;
        color.g = (u8)(230 - s * 100);
        color.b = (u8)(55 - s * 35);
        color.a = 255;
    }
    else
    {
        f64 s = (t - COLOR_THRESHOLD_MID) / COLOR_THRESHOLD_HIGH;
        color.r = (u8)(255 - s * 55);
        color.g = (u8)(130 - s * 90);
        color.b = (u8)(20 - s * 10);
        color.a = 255;
    }

    return color;
}

i32 transforms_init_course_data(app_state_t *app_state)
{
    for (ul i = 0; i < app_state->data_point_count; ++i)
    {
        {
            const f64 right_ascension_rad = (app_state->data_points_a[i].right_ascension / ARCMIN_TO_DEGREES) * DEG2RAD;
            const f64 declination_rad = (app_state->data_points_a[i].declination / ARCMIN_TO_DEGREES) * DEG2RAD;
            const f64 radius = COURSE_DATA_RADIUS;
            const f64 x = radius * cos(right_ascension_rad) * cos(declination_rad);
            const f64 y = radius * sin(declination_rad);
            const f64 z = radius * sin(right_ascension_rad) * cos(declination_rad);

            app_state->matrix_transforms_a[i] = MatrixIdentity();
            app_state->matrix_transforms_a[i] = MatrixMultiply(app_state->matrix_transforms_a[i], MatrixScale(COURSE_DATA_SCALE, COURSE_DATA_SCALE, COURSE_DATA_SCALE));
            app_state->matrix_transforms_a[i] = MatrixMultiply(app_state->matrix_transforms_a[i], MatrixTranslate((f32)x, (f32)y, (f32)z));
        }

        {
            const f64 right_ascension_rad = (app_state->data_points_b[i].right_ascension / ARCMIN_TO_DEGREES) * DEG2RAD;
            const f64 declination_rad = (app_state->data_points_b[i].declination / ARCMIN_TO_DEGREES) * DEG2RAD;
            const f64 radius = COURSE_DATA_RADIUS;
            const f64 x = radius * cos(right_ascension_rad) * cos(declination_rad);
            const f64 y = radius * sin(declination_rad);
            const f64 z = radius * sin(right_ascension_rad) * cos(declination_rad);

            app_state->matrix_transforms_b[i] = MatrixIdentity();
            app_state->matrix_transforms_b[i] = MatrixMultiply(app_state->matrix_transforms_b[i], MatrixScale(COURSE_DATA_SCALE, COURSE_DATA_SCALE, COURSE_DATA_SCALE));
            app_state->matrix_transforms_b[i] = MatrixMultiply(app_state->matrix_transforms_b[i], MatrixTranslate((f32)x, (f32)y, (f32)z));
        }
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
        f64 distance_t = (radius - RENDER_DISTANCE_MIN) / RENDER_DISTANCE_RANGE;
        distance_t = fmax(0.0, fmin(distance_t, 1.0));
        f64 distance_scale = DISTANCE_SIZE_SCALE_MIN + distance_t * (DISTANCE_SIZE_SCALE_MAX - DISTANCE_SIZE_SCALE_MIN);
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
