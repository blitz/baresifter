#include <cstring>

#include "cpuid.hpp"

cpuid_result get_cpuid(uint32_t leaf, uint32_t subleaf)
{
  cpuid_result res;
  asm ("cpuid"
       : "=a" (res.eax), "=b" (res.ebx),
         "=d" (res.edx), "=c" (res.ecx)
       : "a" (leaf), "c" (subleaf));
  return res;
}

cpu_signature get_cpu_signature()
{
  auto leaf0 = get_cpuid(0);
  auto leaf1 = get_cpuid(1);

  cpu_signature sig {};

  sig.signature = leaf1.eax;
  memcpy(sig.vendor + 0, &leaf0.ebx, sizeof(uint32_t));
  memcpy(sig.vendor + 4, &leaf0.edx, sizeof(uint32_t));
  memcpy(sig.vendor + 8, &leaf0.ecx, sizeof(uint32_t));

  return sig;
}

bool running_virtualized()
{
  return get_cpuid(1).ecx & (1U << 31);
}
