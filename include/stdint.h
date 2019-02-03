#pragma once

#define CHAR_BIT 8

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;

#ifdef __x86_64__
typedef unsigned long uint64_t;
typedef signed long int64_t;
#else
typedef unsigned long long uint64_t;
typedef signed long long int64_t;
#endif

typedef unsigned long uintptr_t;

#ifdef __cplusplus
static_assert(sizeof(uint64_t) == 8, "cstdint is broken");
#endif
