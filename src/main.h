#ifndef main_h
#define main_h

#ifdef NDEBUG
#   define Assert(...)
#   define CheckPos(E, ...) E
#   define CheckNeg(E, ...) E
#   define InvalidCodePath __builtin_unreachable()
#   define InvalidDefaultCase
#   define static_assert _Static_assert
#else
#   include <assert.h>
#   define Assert(E) assert(E)
#   define CheckPos(E, V) assert(V == (E))
#   define CheckNeg(E, V) assert(V != (E))
#   include <stdlib.h>
#   define InvalidCodePath abort()
#   define InvalidDefaultCase default: InvalidCodePath;
#endif

#include <stdio.h>
#include <errno.h>
#include <string.h>

#define ArrayCount(A) (sizeof A / sizeof *A)

#include <stdint.h>
typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef intptr_t  sptr;
typedef uintptr_t ptr;

#include <stddef.h>
typedef ptrdiff_t dptr;
typedef size_t    mm;
typedef ssize_t   smm;

#include <stdbool.h>

typedef float  r32;
typedef double r64;

typedef int fd;

/* put the cannary into its cage */
static_assert(sizeof (ptr) == sizeof (sptr));
static_assert(sizeof (dptr) == sizeof (sptr));
static_assert(sizeof (dptr) == sizeof (ptr));
static_assert(sizeof (mm) == sizeof (ptr));
static_assert(sizeof (mm) == sizeof (smm));
static_assert(sizeof (smm) == sizeof (dptr));

#define fallthrough __attribute__((fallthrough))
/*#define macro static __attribute__((always_inline))*/
#define inline inline __attribute((gnu_inline))
#define noreturn _Noreturn

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#endif
