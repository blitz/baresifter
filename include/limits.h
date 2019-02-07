#pragma once

#define UCHAR_MAX 255

#if __SIZEOF_INT__ == 4
# define INT_MAX  2147483647
#endif

#if __SIZEOF_LONG__ == 4
# define LONG_MAX 2147483647L
#elif __SIZEOF_LONG__ == 8
# define LONG_MAX 9223372036854775807L
#endif

#define INT_MIN (-INT_MAX - 1)
#define LONG_MIN (-LONG_MAX - 1L)

#define weak_alias(a, b)
