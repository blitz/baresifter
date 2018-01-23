#pragma once

#include <cstdint>

// Immediate 8-bit OUT operation
template <int PORT>
inline void outbi(uint8_t data)
{
  static_assert(PORT >= 0 and PORT < 0x100, "Port is out of range");
  __asm__ __volatile__ ("outb %%al, %0" :: "N" (PORT), "a" (data));
}

// Generic 8-bit OUT operation
inline void outb(uint16_t port, uint8_t data)
{
  __asm__ __volatile__ ("outb %%al, (%%dx)" :: "d" (port), "a" (data));
}
