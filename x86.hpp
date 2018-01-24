#pragma once

#include <cstdint>

constexpr uint16_t ring0_code_selector = 0x10;
constexpr uint16_t ring0_data_selector = 0x20;
constexpr uint16_t ring0_tss_selector = 0x30;

// Immediate 8-bit OUT operation
template <int PORT>
inline void outbi(uint8_t data)
{
  static_assert(PORT >= 0 and PORT < 0x100, "Port is out of range");
  asm volatile ("outb %%al, %0" :: "N" (PORT), "a" (data));
}

// Generic 8-bit OUT operation
inline void outb(uint16_t port, uint8_t data)
{
  asm volatile ("outb %%al, (%%dx)" :: "d" (port), "a" (data));
}

[[noreturn]] inline void wait_forever()
{
  while (true)
    asm volatile ("cli ; hlt");
}
