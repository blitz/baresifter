#include "stdlib.hpp"

__attribute__((used)) void *memset(void *s, int c, size_t n)
{
  void *dest = s;
  asm ("rep stosb" : "+D" (dest) : "a" (c), "c" (n) : "memory");
  return s;
}
