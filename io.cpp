#include <cstdint>

#include "io.hpp"
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
