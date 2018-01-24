#pragma once

#include <cstdint>

#define unlikely(x) __builtin_expect(!!x, 0)

void print_char(char c);
void print_string(const char *s);
void print_hex(uint64_t v);

[[noreturn]] void fail_assert(const char *s);

inline void assert(bool cnd, const char *s)
{
  if (unlikely(not cnd)) {
    fail_assert(s);
  }
}

// Disable interrupts and halt the CPU.
extern "C" [[noreturn]] void wait_forever();
