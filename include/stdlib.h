#pragma once

#include "compiler.h"
#include <stddef.h>

#define offsetof(type, field) __builtin_offsetof (type, field)

EXTERN_C void qsort(void *base, size_t nmemb, size_t size,
                    int (*compar)(const void *, const void *));

EXTERN_C int atoi(const char *s);
