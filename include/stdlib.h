#pragma once

#include "compiler.h"
#include <stddef.h>

#define offsetof(type, field) __builtin_offsetof (type, field)

void qsort(void *base, size_t nmemb, size_t size,
           int (*compar)(const void *, const void *));
