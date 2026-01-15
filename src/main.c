#include "app.h"

i32 main(i32 argc, char **argv)
{
    app_state_t *app = app_create();
    if (!app)
    {
        return 1;
    }

    app_parse_args(app, argc, argv);

    if (app_init(app) != 0)
    {
        return 1;
    }

    app_run(app);
    app_cleanup(app);

    return 0;
}
