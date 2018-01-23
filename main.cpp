#include <cstdint>

#include "io.hpp"

extern "C" void start();
extern "C" void (*_init_array_start[])();
extern "C" void (*_init_array_end[])();

void start()
{
  print_string(">>> Executing constructors.\n");
  for (auto p = _init_array_start; p < _init_array_end; p++)
    (*p)();
}
