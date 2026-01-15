#ifndef TRANSFORMS_H
#define TRANSFORMS_H

#include "data_types.h"

f64 transforms_distance_from_velocity(f64 velocity_km_s);
Color transforms_color_from_velocity(f64 velocity_km_s);
i32 transforms_init_course_data(app_state_t *app_state);
i32 transforms_init_redshift_data(app_state_t *app_state);
i32 transforms_upload_to_gpu(app_state_t *app_state);

#endif
