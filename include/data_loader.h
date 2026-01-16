#ifndef DATA_LOADER_H
#define DATA_LOADER_H

#include "data_types.h"

// Speed of light in km/s for redshift to velocity conversion
#define SPEED_OF_LIGHT_KMS 299792.458

i32 data_loader_load_all(app_state_t *app_state);
usize data_loader_read_arcmin_file(const char *file_name, arcmin_data_t *data_points, ul max_points);
usize data_loader_read_redshift_file(const char *file_name, redshift_galaxy_t *galaxies, ul max_galaxies);
usize data_loader_read_saga_dr3_file(const char *file_name, redshift_galaxy_t *galaxies, ul max_galaxies);

#endif // DATA_LOADER_H
