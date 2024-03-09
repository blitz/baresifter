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

uint32_t get_cpuid_max_std_level()
{
  return get_cpuid(0).eax;
}

uint32_t get_cpuid_max_ext_level()
{
  return get_cpuid(0x80000000).eax;
}

bool has_nx()
{
  return get_cpuid_max_ext_level() >= 0x80000001
    and (get_cpuid(0x80000001).edx & (1 << 20));
}

bool has_smep()
{
    return get_cpuid_max_std_level() >= 7
        and (get_cpuid(0x7).ebx & (1 << 7));
}
