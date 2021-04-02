#pragma once

#include <cstddef>
#include <cstdint>

#define unlikely(x) __builtin_expect(!!x, 0)

// Return the number of entries of a C-style array.
template <typename T, size_t N>
constexpr size_t array_size(T(&)[N]) { return N; }

// Return the bits in the range from high (non-inclusive) to low (inclusive)
// extracted from value.
inline uint64_t bit_select(int high, int low, uint64_t value)
{
  return (value >> low) & ((1UL << (high - low)) - 1);
}

void print(const char *s);

struct formatted_int {
  uint64_t v;
  unsigned base;
  unsigned width;
  bool prefix;

  formatted_int(uint64_t v_, unsigned base_ = 10, unsigned width_ = 0, bool prefix_ = false)
    : v(v_), base(base_), width(width_), prefix(prefix_) {}
};

inline formatted_int hex(uint64_t v, unsigned width = 0, bool prefix = true)
{
  return { v, 16, width, prefix };
}

//void print(uint64_t v);
void print(formatted_int const &v);

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
extern "C" void execute_constructors();
