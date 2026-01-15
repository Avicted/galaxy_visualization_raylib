#include "utils.h"
#include "macros.h"
#include "includes.h"

const char *
format_u64_suffix(u64 value)
{
    local_persist char buffers[4][32];
    local_persist i32 buffer_index = 0;

    char *buf = buffers[buffer_index++ & 3];

    const char *suffixes[] = {"", "k", "M", "B", "T", "P", "E"};
    const u64 thresholds[] = {
        1,
        1000,
        1000000,
        1000000000,
        1000000000000ULL,
        1000000000000000ULL,
        1000000000000000000ULL,
    };

    i32 scale = 0;
    while (scale < 6 && value >= thresholds[scale + 1])
    {
        scale++;
    }

    if (scale == 0)
    {
        snprintf(buf, 32, "%llu", (unsigned long long)value);
        return buf;
    }

    u64 denom = thresholds[scale];
    u64 whole = value / denom;
    u64 rem = value % denom;

    // Show one decimal for values < 10 in scaled unit (e.g. 1.2k, 2.5M)
    if (whole < 10)
    {
        u64 decimal = (rem * 10) / denom;
        if (decimal > 0)
        {
            snprintf(buf, 32, "%llu.%llu%s",
                     (unsigned long long)whole,
                     (unsigned long long)decimal,
                     suffixes[scale]);
        }
        else
        {
            snprintf(buf, 32, "%llu%s", (unsigned long long)whole, suffixes[scale]);
        }
    }
    else
    {
        snprintf(buf, 32, "%llu%s", (unsigned long long)whole, suffixes[scale]);
    }

    return buf;
}

void print_memory_usage(app_state_t *app_state)
{
    printf("[INFO]  Memory: %.2fMB\n", (f64)app_state->cpu_memory_allocated / (f64)MegaBytes(1));
}
