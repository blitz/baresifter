#pragma once

#include <cstddef>
#include <cstdint>

#define unlikely(x) __builtin_expect(!!x, 0)

// Return the number of entries of a C-style array.
template <typename T, size_t N>
constexpr size_t array_size(T(&)[N]) { return N; }

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
