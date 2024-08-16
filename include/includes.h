#pragma once

// Standard library
#include <iostream>
#include <signal.h>
#include <string>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

// If Linux
#ifdef __linux__
#include <string.h>
#include <sys/time.h>
#endif

// If Windows
#ifdef _WIN32
// Nothing to include
#endif

// Program specific stuff --------------------------------------
#define local_persist static   // localy scoped persisted variable
#define global_variable static // globaly scoped variable in the same translation unit
#define internal static        // localy scoped function to the translation unit

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef size_t usize;
typedef intmax_t isize;

typedef float f32;
typedef double f64;

// Macros ------------------------
// @Note(Victor): Write straight to the null pointer to crash the program
// @Note(Victor): *(int *)0 = 0;
#define Assert(Expression) \
    if (!(Expression))     \
    {                      \
        __builtin_trap();  \
    }

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#define Kilobytes(Value) ((Value) * 1024LL)
#define Megabytes(Value) (Kilobytes(Value) * 1024LL)
#define Gigabytes(Value) (Megabytes(Value) * 1024LL)
