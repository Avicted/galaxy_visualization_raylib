#ifndef CAMERA_H
#define CAMERA_H

#include "data_types.h"

void camera_handle_resize(app_state_t *app_state);
void camera_update(app_state_t *app_state, f64 dt);

#endif // CAMERA_H
