#include "util.hpp"
#include "x86.hpp"

extern "C" void (*_init_array_start[])();
extern "C" void (*_init_array_end[])();

static constexpr uint16_t qemu_debug_port = 0xe9;

static void print_char(char c)
{
  outbi<qemu_debug_port>(c);
}

void print(const char *str)
{
  for (; *str; str++)
    print_char(*str);
}

void print(formatted_int const &v) {
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

  if (v.prefix)
    print("0x");

  do {
    print_char(*(--p));
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
