#ifndef MACROS_H
#define MACROS_H

#define ASSERT(x)                                                                   \
    if (!(x))                                                                       \
    {                                                                               \
        printf("Assertion failed: %s, file %s, line %d\n", #x, __FILE__, __LINE__); \
        exit(1);                                                                    \
    }

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define KiloBytes(Value) (Value << 10LL)
#define MegaBytes(Value) (Value << 20LL)
#define GigaBytes(Value) (Value << 30LL)

#endif // MACROS_H
