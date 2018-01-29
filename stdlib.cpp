#include <string.h>

// We have to mark these functions as used, because calls to them are generated
// by the compiler.
#define used __attribute__((used))

used void *memset(void *s, int c, size_t n)
{
  void *dest = s;
  asm ("rep stosb" : "+D" (dest), "+c" (n), "=m" (s) : "a" (c));
  return s;
}

used void *memcpy(void * __restrict__ d, const void * __restrict__ s, size_t n)
{
  void *dest = d;
  asm ("rep movsb" : "+D" (dest), "+S" (s), "+c" (n), "=m" (d) : "m" (s));
  return d;
}

size_t strlen(const char *s)
{
  size_t i = 0;
  for (; s[i] != 0; i++);
  return i;
}
