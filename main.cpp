#include <cstdint>

#include "io.hpp"

extern "C" void start();

void start()
{
  print_string("Hello World!");
}
