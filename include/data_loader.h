#ifndef DATA_LOADER_H
#define DATA_LOADER_H

#include "data_types.h"

i32 data_loader_load_all(app_state_t *app_state);
usize data_loader_read_arcmin_file(const char *file_name, arcmin_data_t *data_points, ul max_points);
usize data_loader_read_redshift_file(const char *file_name, redshift_galaxy_t *galaxies, ul max_galaxies);

#endif
