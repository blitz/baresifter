#include "util.hpp"
#include "x86.hpp"

static constexpr uint16_t qemu_debug_port = 0xe9;

void print_char(char c)
{
  outbi<qemu_debug_port>(c);
}

void print_string(const char *str)
{
  for (; *str; str++)
    print_char(*str);
}

void print_hex(uint64_t v) {
  static const char hexdigit[] = "0123456789ABCDEF";

  char output[16];
  char *p = output;

  do {
    *(p++) = hexdigit[v & 0xF];
    v = v >> 4;
  } while (v != 0);

  do {
    print_char(*(--p));
  } while (p != output);
}

void fail_assert(const char *s)
{
  print_string("Assertion failed: ");
  print_string(s);
  print_char('\n');
  wait_forever();
}

void wait_forever()
{
  while (true)
    asm volatile ("cli ; hlt");
}
