#pragma once

#include <cstddef>
#include <cstdint>

#define unlikely(x) __builtin_expect(!!x, 0)

// Return the number of entries of a C-style array.
template <typename T, size_t N>
constexpr size_t array_size(T(&)[N]) { return N; }

void print(const char *s);
void print(uint64_t v);

template <typename T>
void format(T const &t)
{
  print(t);
}

// TODO This bloats the binary a bit for clang++ with LTO, because everyting
// including the print functions are inlined.
template <typename FIRST, typename SECOND, typename... REST>
void format(FIRST const &f, SECOND const &s, REST const &... r)
{
  print(f);
  format((s), r...);
}

[[noreturn]] void fail_assert(const char *s);

inline void assert(bool cnd, const char *s)
{
  if (unlikely(not cnd)) {
    fail_assert(s);
  }
}

// Disable interrupts and halt the CPU.
extern "C" [[noreturn]] void wait_forever();

// Execute constructors
void execute_constructors();
