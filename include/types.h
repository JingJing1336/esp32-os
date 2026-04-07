#ifndef TYPES_H
#define TYPES_H

typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;

typedef signed char        int8_t;
typedef signed short       int16_t;
typedef signed int         int32_t;
typedef signed long long   int64_t;

typedef uint32_t           size_t;
typedef int32_t            ptrdiff_t;
typedef uint32_t           uintptr_t;
typedef int32_t            intptr_t;

typedef enum {
    false = 0,
    true  = 1
} bool;

#define NULL ((void *)0)

#define STATIC_ASSERT(cond) typedef char static_assert_##__LINE__[(cond) ? 1 : -1]

STATIC_ASSERT(sizeof(uint8_t)  == 1);
STATIC_ASSERT(sizeof(uint32_t) == 4);

#endif /* TYPES_H */
