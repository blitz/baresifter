#include <cstdint>

#include "idt.hpp"
#include "util.hpp"

extern "C" void start();
extern "C" void (*_init_array_start[])();
extern "C" void (*_init_array_end[])();

void start()
{
  format(">>> Executing constructors.\n");
  for (auto p = _init_array_start; p < _init_array_end; p++)
    (*p)();

  format(">>> Setting up IDT.\n");
  setup_idt();

  format(">>> Causing an exception.\n");
  asm volatile ("lea 1f, %%rdi ; ud2 ; 1:" ::: "rdi");
  format(">>> We're back!\n");
}
