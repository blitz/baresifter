#include "cpuid.hpp"
#include "avx.hpp"
#include "x86.hpp"
#include "util.hpp"

void try_setup_avx()
{
  auto const xsave_leaf = get_cpuid(0xD);
  uint64_t const valid_xcr0 = (uint64_t)xsave_leaf.edx << 32 | xsave_leaf.eax;

  if ((get_cpuid(1).ecx & (1 << 28 /* AVX */)) and (valid_xcr0 & (1 << 2 /* AVX */))) {
    format(">>> Enabling AVX.\n");
    set_xcr0(get_xcr0() | (1 << 2));
  }
}
