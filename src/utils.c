#include "utils.h"
#include "macros.h"
#include "includes.h"

const char *
format_u64_thousands_dots(u64 value)
{
    local_persist char buffers[4][32];
    local_persist i32 buffer_index = 0;

    char *buf = buffers[buffer_index++ & 3];
    char *out = buf + 31;
    *out = '\0';

    if (value == 0)
    {
        *--out = '0';
        return out;
    }

    i32 group = 0;
    while (value > 0)
    {
        if (group == 3)
        {
            *--out = '.';
            group = 0;
        }
        *--out = (char)('0' + (value % 10));
        value /= 10;
        group++;
    }

    return out;
}

void print_memory_usage(app_state_t *app_state)
{
    printf("[INFO]  Memory: %.3f MB\n", (f64)app_state->cpu_memory_allocated / (f64)Megabytes(1));
}
