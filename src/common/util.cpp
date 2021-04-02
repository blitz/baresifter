#include "output_device.hpp"
#include "util.hpp"
#include "x86.hpp"

extern "C" void (*_init_array_start[])();
extern "C" void (*_init_array_end[])();

static output_device * const output_device = output_device::make();

void print(const char *str)
{
  output_device->puts(str);
}

__attribute__((noinline)) void print(formatted_int const &v)
{
  static const char hexdigit[] = "0123456789ABCDEF";
  assert(v.base <= sizeof(hexdigit), "Invalid base");

  char output[64];
  assert(v.width <= sizeof(output), "Invalid width");

  char *p = output;
  uint64_t value = v.v;
  unsigned width = v.width;

  do {
    *(p++) = hexdigit[value % v.base];
    if (width) width--;

    value = value / v.base;
  } while (value != 0 or width);

  if (v.prefix and v.base == 16)
    print("0x");

  do {
    output_device->putc(*(--p));
  } while (p != output);
}

void fail_assert(const char *s)
{
  format("Assertion failed: ", s, "\n");
  wait_forever();
}

void wait_forever()
{
  while (true)
    asm volatile ("cli ; hlt");
}

void execute_constructors()
{
  for (auto p = _init_array_start; p < _init_array_end; p++)
    (*p)();
}
