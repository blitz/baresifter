#include <cstring>

#include "cpuid.hpp"

bool cpuid_supported()
{
    #ifndef __x86_64__
    //We need to check for the existence of CPUID on 32-bit platforms!
    unsigned int res1,res2;
    asm(
        "pushfd\n\t" //Save original interrupt state
        "cli\n\t" //Block interrupts to be safe, as we're modifying the stack alignment, making this a critical section
        "push %%ebp\n\t" //Save original stack base pointer
        "mov %%esp,%%ebp\n\t" //Save original stack alignment
        "and -4,%%esp\n\t" //align stack
        "pushfd\n\t" //Load...
        "pop %%eax\n\t" //... old EFLAGS
        "mov %%ebx,%%eax\n\t" //Copy of it for the result check
        "xor %%eax,$200000\n\t" //Flip CPUID bit now
        "push %%eax\n\t"
        "popfd\n\t" //Store changed bit into flags
        "pushfd\n\t" //New eflags back on the stack
        "pop %%eax\n\t" //Get if it changed
        "mov %%eax, %0\n\t" //Original eflags result
        "mov %%ebx, %1\n\t" //Flipped eflags result
        "mov %%ebp,%%esp\n\t" //Restore original stack alignment
        "pop %%ebp\n\t" //Restore stack base pointer
        "popfd" //Restore original interrupt state
        : "=a" (res1), "=b" (res2));
    return (((res1 ^ res2) & 0x200000)!=0); //Has the CPUID bit changed and is supported?
    #else
    return true; //Always assumed supported!
    #endif
}

cpuid_result get_cpuid(uint32_t leaf, uint32_t subleaf)
{
  cpuid_result res;
  if (!cpuid_supported()) //CPUID not supported?
  {
      res.eax = res.ebx = res.edx = res.ecx = 0; //Simply give empty result!
      return res; //Give empty result!
  }

  //CPUID is supported!
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

bool has_pse()
{
    return get_cpuid_max_std_level() >= 1
        and (get_cpuid(0x1).edx & (1 << 3));
}