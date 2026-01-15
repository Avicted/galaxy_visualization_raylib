#ifndef APP_H
#define APP_H

#include "data_types.h"

app_state_t *app_create(void);
void app_parse_args(app_state_t *app_state, i32 argc, char **argv);
i32 app_init(app_state_t *app_state);
void app_run(app_state_t *app_state);
void app_cleanup(app_state_t *app_state);

#endif
