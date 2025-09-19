#ifndef REDEFINES_H
#define REDEFINES_H

#include <stddef.h>
#include <stdint.h>

#define local_persist static   // localy scoped persisted variable
#define global_variable static // globaly scoped variable in the same translation unit
#define internal static        // localy scoped function to the translation unit

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef unsigned long ul;
typedef unsigned long long ull;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef size_t usize;
typedef intmax_t isize;

typedef float f32;
typedef double f64;

#endif // REDEFINES_H
