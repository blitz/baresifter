#include <cstdint>

extern "C" void start();

void start()
{
  // Print ! to the serial console as a sign of life.
  asm volatile ("out %%al, (%%dx)" : : "d" (0x3f8), "a" ('!'));
}
