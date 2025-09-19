#ifndef MACROS_H
#define MACROS_H

#define ASSERT(x)                                                                   \
    if (!(x))                                                                       \
    {                                                                               \
        printf("Assertion failed: %s, file %s, line %d\n", #x, __FILE__, __LINE__); \
        exit(1);                                                                    \
    }

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#define Kilobytes(Value) ((Value) * 1024LL)
#define Megabytes(Value) (Kilobytes(Value) * 1024LL)
#define Gigabytes(Value) (Megabytes(Value) * 1024LL)

#endif // MACROS_H
