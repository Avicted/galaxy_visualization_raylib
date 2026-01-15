#include "data_loader.h"
#include "includes.h"
#include "macros.h"

global_variable const char *DATA_A_FILENAME = "./input_data/data_100k_arcmin.txt";
global_variable const char *DATA_B_FILENAME = "./input_data/flat_100k_arcmin.txt";
global_variable const char *REDSHIFT_DATA_FILENAME = "./input_data/redshift_input_data/seyfert.dat";
global_variable const char *SAGA_DR3_FILENAME = "./input_data/redshift_input_data/saga-dr3-satellites.txt";

usize data_loader_read_arcmin_file(const char *file_name, arcmin_data_t *data_points, ul max_points)
{
    FILE *file = fopen(file_name, "r");
    if (file == NULL)
    {
        fprintf(stderr, "[ERROR] Cannot open: %s\n", file_name);
        return -1;
    }

    char line[1024];

    if (fgets(line, sizeof(line), file) == NULL)
    {
        fprintf(stderr, "[ERROR] Cannot read header: %s\n", file_name);
        fclose(file);
        return -1;
    }

    ul i = 0;
    while (i < max_points && fgets(line, sizeof(line), file) != NULL)
    {
        usize len = strlen(line);
        if (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
        {
            line[--len] = '\0';
        }
        if (len == 0)
        {
            continue;
        }

        char *p = line;
        char *endptr = NULL;

        f64 ra = strtod(p, &endptr);
        if (endptr == p)
        {
            fprintf(stderr, "[ERROR] Parse RA failed at line %lu: %s\n", (ul)i + 2, file_name);
            fclose(file);
            return -1;
        }

        p = endptr;
        while (*p == '\t' || *p == ' ' || *p == ',')
        {
            ++p;
        }

        f64 dec = strtod(p, &endptr);
        if (endptr == p)
        {
            fprintf(stderr, "[ERROR] Parse DEC failed at line %lu: %s\n", (ul)i + 2, file_name);
            fclose(file);
            return -1;
        }

        data_points[i].right_ascension = ra;
        data_points[i].declination = dec;
        ++i;
    }

    fclose(file);
    return (usize)i;
}

usize data_loader_read_redshift_file(const char *file_name, redshift_galaxy_t *galaxies, ul max_galaxies)
{
    FILE *file = fopen(file_name, "r");
    if (file == NULL)
    {
        fprintf(stderr, "[ERROR] Cannot open: %s\n", file_name);
        return 0;
    }

    char line[256];
    ul galaxy_count = 0;
    ul line_number = 0;

    while (line_number < 14 && fgets(line, sizeof(line), file) != NULL)
    {
        line_number++;
    }

    while (galaxy_count < max_galaxies && fgets(line, sizeof(line), file) != NULL)
    {
        line_number++;
        usize len = strlen(line);

        if (len < 30)
        {
            continue;
        }
        if (line[0] == '-')
        {
            continue;
        }
        if (line[0] == '\n' || line[0] == '\r')
        {
            continue;
        }

        char line_copy[256];
        strncpy(line_copy, line, 255);
        line_copy[255] = '\0';

        char *tokens[20] = {0};
        i32 token_count = 0;
        char *token = strtok(line_copy, " \t\n\r");
        while (token != NULL && token_count < 20)
        {
            tokens[token_count++] = token;
            token = strtok(NULL, " \t\n\r");
        }

        if (token_count < 5)
        {
            continue;
        }

        strncpy(galaxies[galaxy_count].name, tokens[0], 15);
        galaxies[galaxy_count].name[15] = '\0';

        char *ra_str = tokens[1];
        usize ra_len = strlen(ra_str);
        if (ra_len < 6)
        {
            continue;
        }

        char ra_h_str[3] = {ra_str[0], ra_str[1], '\0'};
        char ra_m_str[3] = {ra_str[2], ra_str[3], '\0'};
        char ra_s_str[8] = {0};
        strncpy(ra_s_str, ra_str + 4, 7);

        i32 ra_hours = atoi(ra_h_str);
        i32 ra_minutes = atoi(ra_m_str);
        f64 ra_seconds = strtod(ra_s_str, NULL);

        f64 ra_decimal_hours = (f64)ra_hours + (f64)ra_minutes / 60.0 + ra_seconds / 3600.0;
        galaxies[galaxy_count].right_ascension = ra_decimal_hours * 15.0;

        char *dec_str = tokens[2];
        usize dec_len = strlen(dec_str);
        if (dec_len < 5)
        {
            continue;
        }

        bool is_negative = (dec_str[0] == '-');
        i32 dec_start = (dec_str[0] == '-' || dec_str[0] == '+') ? 1 : 0;

        char dec_d_str[3] = {dec_str[dec_start], dec_str[dec_start + 1], '\0'};
        char dec_m_str[3] = {dec_str[dec_start + 2], dec_str[dec_start + 3], '\0'};
        char dec_s_str[3] = {0};
        if (dec_len >= (usize)(dec_start + 6))
        {
            dec_s_str[0] = dec_str[dec_start + 4];
            dec_s_str[1] = dec_str[dec_start + 5];
        }

        i32 dec_degrees = atoi(dec_d_str);
        i32 dec_minutes = atoi(dec_m_str);
        i32 dec_seconds = atoi(dec_s_str);

        f64 dec_decimal = (f64)dec_degrees + (f64)dec_minutes / 60.0 + (f64)dec_seconds / 3600.0;
        if (is_negative)
        {
            dec_decimal = -dec_decimal;
        }
        galaxies[galaxy_count].declination = dec_decimal;

        galaxies[galaxy_count].b_magnitude = strtod(tokens[3], NULL);

        i32 velocity = atoi(tokens[4]);
        if (velocity <= 500)
        {
            continue;
        }

        galaxies[galaxy_count].helio_velocity = (f64)velocity;
        galaxy_count++;
    }

    fclose(file);
    printf("[INFO]  Loaded %lu redshift galaxies\n", galaxy_count);
    return galaxy_count;
}

usize data_loader_read_saga_dr3_file(const char *file_name, redshift_galaxy_t *galaxies, ul max_galaxies)
{
    FILE *file = fopen(file_name, "r");
    if (file == NULL)
    {
        fprintf(stderr, "[ERROR] Cannot open: %s\n", file_name);
        return 0;
    }

    char line[512];
    ul galaxy_count = 0;
    ul line_number = 0;

    // Skip header lines until we find the data separator line
    // The header ends after the "---" line (line 23 in the file)
    while (fgets(line, sizeof(line), file) != NULL)
    {
        line_number++;
        // Data starts after line 23 (after the byte description table)
        if (line_number >= 23)
        {
            break;
        }
    }

    // Now read the actual data
    while (galaxy_count < max_galaxies && fgets(line, sizeof(line), file) != NULL)
    {
        line_number++;
        usize len = strlen(line);

        // Skip empty lines
        if (len < 50)
        {
            continue;
        }

        // Parse the fixed-width format according to the byte description:
        // Bytes  1-18:  OBJID (unique galaxy identifier)
        // Bytes 20-28:  HOSTID
        // Bytes 30-36:  PGC
        // Bytes 38-48:  RAdeg (Right ascension in decimal degrees)
        // Bytes 50-60:  DEdeg (Declination in decimal degrees)
        // Bytes 62-67:  rmag (apparent r-band magnitude)
        // ... more columns ...
        // Bytes 126-132: z (Spectroscopic redshift, -1 if not measured)

        char line_copy[512];
        strncpy(line_copy, line, 511);
        line_copy[511] = '\0';

        char *tokens[20] = {0};
        i32 token_count = 0;
        char *token = strtok(line_copy, " \t\n\r");
        while (token != NULL && token_count < 20)
        {
            tokens[token_count++] = token;
            token = strtok(NULL, " \t\n\r");
        }

        // Need at least: OBJID, HOSTID, PGC, RA, DEC, rmag, e_rmag, gr, rmag-fiber, sb, ba, PA, Sersic, TELNAME, z
        // That's 15 tokens minimum
        if (token_count < 15)
        {
            continue;
        }

        // Extract redshift (token 14, 0-indexed)
        f64 redshift = strtod(tokens[14], NULL);

        // Skip invalid redshifts (-1 means not measured)
        if (redshift <= 0.0 || redshift > 1.0)
        {
            continue;
        }

        // Convert redshift to heliocentric velocity: v = z * c
        f64 velocity = redshift * SPEED_OF_LIGHT_KMS;

        // Skip very nearby galaxies (velocity threshold similar to seyfert parser)
        if (velocity <= MIN_VELOCITY_THRESHOLD)
        {
            continue;
        }

        // Extract RA and DEC (already in decimal degrees)
        f64 ra = strtod(tokens[3], NULL);
        f64 dec = strtod(tokens[4], NULL);

        // Validate coordinates
        if (ra < 0.0 || ra > 360.0 || dec < -90.0 || dec > 90.0)
        {
            continue;
        }

        // Extract magnitude (rmag, token 5)
        f64 magnitude = strtod(tokens[5], NULL);

        // Build galaxy name from OBJID (first 15 chars)
        strncpy(galaxies[galaxy_count].name, tokens[0], 15);
        galaxies[galaxy_count].name[15] = '\0';

        galaxies[galaxy_count].right_ascension = ra;
        galaxies[galaxy_count].declination = dec;
        galaxies[galaxy_count].helio_velocity = velocity;
        galaxies[galaxy_count].b_magnitude = magnitude;

        galaxy_count++;
    }

    fclose(file);
    printf("[INFO]  Loaded %lu SAGA DR3 galaxies\n", galaxy_count);
    return galaxy_count;
}

i32 data_loader_load_all(app_state_t *app_state)
{
    app_state->data_points_a = (arcmin_data_t *)calloc(MAX_DATA_POINTS, sizeof(arcmin_data_t));
    if (app_state->data_points_a == NULL)
    {
        fprintf(stderr, "[ERROR] Alloc failed: data_points_a\n");
        return 1;
    }
    app_state->cpu_memory_allocated += (usize)MAX_DATA_POINTS * sizeof(arcmin_data_t);

    app_state->data_points_b = (arcmin_data_t *)calloc(MAX_DATA_POINTS, sizeof(arcmin_data_t));
    if (app_state->data_points_b == NULL)
    {
        fprintf(stderr, "[ERROR] Alloc failed: data_points_b\n");
        free(app_state->data_points_a);
        app_state->cpu_memory_allocated -= (usize)MAX_DATA_POINTS * sizeof(arcmin_data_t);
        app_state->data_points_a = NULL;
        return 1;
    }
    app_state->cpu_memory_allocated += (usize)MAX_DATA_POINTS * sizeof(arcmin_data_t);

    usize count_a = data_loader_read_arcmin_file(DATA_A_FILENAME, app_state->data_points_a, MAX_DATA_POINTS);
    if (count_a == 0)
    {
        fprintf(stderr, "[ERROR] Read failed: %s\n", DATA_A_FILENAME);
        return 1;
    }

    usize count_b = data_loader_read_arcmin_file(DATA_B_FILENAME, app_state->data_points_b, MAX_DATA_POINTS);
    if (count_b == 0)
    {
        fprintf(stderr, "[ERROR] Read failed: %s\n", DATA_B_FILENAME);
        return 1;
    }

    if ((usize)count_a != (usize)count_b)
    {
        fprintf(stderr, "[WARN]  Dataset count mismatch: %zd vs %zd\n", count_a, count_b);
    }

    app_state->data_point_count = (ul)MIN((usize)count_a, (usize)count_b);

    if (app_state->data_point_count == 0)
    {
        fprintf(stderr, "[ERROR] No data loaded\n");
        return 1;
    }

    app_state->redshift_galaxies = (redshift_galaxy_t *)calloc(MAX_REDSHIFT_GALAXIES, sizeof(redshift_galaxy_t));
    if (app_state->redshift_galaxies == NULL)
    {
        fprintf(stderr, "[ERROR] Alloc failed: redshift_galaxies\n");
        return 1;
    }
    app_state->cpu_memory_allocated += (usize)MAX_REDSHIFT_GALAXIES * sizeof(redshift_galaxy_t);

    // Load Seyfert redshift data first
    usize redshift_count = data_loader_read_redshift_file(REDSHIFT_DATA_FILENAME, app_state->redshift_galaxies, MAX_REDSHIFT_GALAXIES);
    printf("[INFO]  Seyfert galaxies: %zu\n", redshift_count);

    // Load SAGA DR3 data and append to the array
    usize remaining_space = MAX_REDSHIFT_GALAXIES - redshift_count;
    if (remaining_space > 0)
    {
        usize saga_count = data_loader_read_saga_dr3_file(SAGA_DR3_FILENAME,
                                                          &app_state->redshift_galaxies[redshift_count],
                                                          remaining_space);
        redshift_count += saga_count;
    }

    app_state->redshift_galaxy_count = (ul)redshift_count;

    if (app_state->redshift_galaxy_count == 0)
    {
        fprintf(stderr, "[WARN]  No redshift galaxies loaded\n");
    }
    else
    {
        printf("[INFO]  Redshift galaxies ready with 3D depth\n");
    }

    app_state->data_is_loaded = true;
    return 0;
}
