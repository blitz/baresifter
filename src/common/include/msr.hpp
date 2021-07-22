#pragma once

#include <cstddef>
#include <cstdint>

enum : uint32_t {
  IA32_EFER = 0xC0000080,
};

enum : uint64_t {
  IA32_EFER_NXE = 1 << 11,
};

inline uint64_t rdmsr(uint32_t index)
{
  uint32_t hi, lo;
  asm volatile ("rdmsr" : "=a" (lo), "=d" (hi) : "c" (index));

  return (uint64_t)hi << 32 | lo;
}

inline void wrmsr(uint32_t index, uint64_t value)
{
  uint32_t hi = value >> 32;
  uint32_t lo = (uint32_t)value;

  asm volatile ("wrmsr" :: "a" (lo), "d" (hi), "c" (index));
}
