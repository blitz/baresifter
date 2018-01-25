#include <cstdint>

#include "paging.hpp"

// These are our boot page table structures, which are partly setup by the
// assembler startup code.
extern "C" uint64_t boot_pml4[512]; // Covers 0 - 512GB
extern "C" uint64_t boot_pdpt[512]; // Covers 0 - 1GB
extern "C" uint64_t boot_pd[512];   // Covers the backing store of this binary (usually 2M - 4M)

alignas(4096) uint64_t boot_pml4[512];
alignas(4096) uint64_t boot_pdpt[512];
alignas(4096) uint64_t boot_pd[512];

void setup_paging()
{
  // TODO Set up our user code page.
  *(volatile int *)0xdeadbeef = 1;
}
