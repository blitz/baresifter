#pragma once

#include "x86.hpp"

void print_char(char c);
void print_string(const char *s);
void print_hex(uint64_t v);

inline void assert(bool cnd, const char *s)
{
  if (not cnd) {
    print_string("Assertion failed: ");
    print_string(s);
    print_char('\n');
    wait_forever();
  }
}
