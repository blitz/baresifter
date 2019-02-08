#pragma once

#include "compiler.h"
#include <stddef.h>

EXTERN_C void *memset(void *s, int c, size_t n);
EXTERN_C void *memcpy(void * __restrict__ d, const void * __restrict__ s, size_t n);
EXTERN_C void *memmove(void *dest, const void *src, size_t n);
EXTERN_C char *strncpy(char *dest, const char *src, size_t n);
EXTERN_C size_t strlen(const char *s);
EXTERN_C int strcmp(const char *s1, const char *s2);
EXTERN_C int strncmp(const char *s1, const char *s2, size_t n);
EXTERN_C char *__strchrnul(const char *s, int c);
EXTERN_C size_t strcspn(const char *s, const char *c);
EXTERN_C size_t strspn(const char *s, const char *c);
EXTERN_C char *strtok(char *s, const char *sep);
