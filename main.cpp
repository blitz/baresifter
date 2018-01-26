#include <cstdint>

#include "idt.hpp"
#include "paging.hpp"
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

  format(">>> Setting up paging.\n");
  setup_paging();

  format(">>> Done!\n");
}
