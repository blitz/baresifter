#include "arch.hpp"
#include "paging.hpp"
#include "util.hpp"
#include "x86.hpp"

extern char _image_end[];

uintptr_t get_user_page()
{
  return (1UL << 32);
}

// These are our boot page table structures, which are partly setup by the
// assembler startup code.
extern "C" uint64_t boot_pml4[512];
extern "C" uint64_t boot_pdpt[512];
extern "C" uint64_t boot_pd[512];

alignas(page_size) uint64_t boot_pml4[512];
alignas(page_size) uint64_t boot_pdpt[512]; // Covers 0 - 512GB
alignas(page_size) uint64_t boot_pd[512];   // Covers 0 - 1GB

alignas(page_size) static uint64_t user_pd[512]; // Covers 4GB-5GB
alignas(page_size) static uint64_t user_pt[512]; // Covers 4GB to 4GB+4K

alignas(page_size) static char user_page_backing[page_size];

char *get_user_page_backing()
{
  return user_page_backing;
}

static uint64_t bit_select(int high, int low, uint64_t value)
{
  return (value >> low) & ((1UL << (high - low)) - 1);
}

void setup_paging()
{
  const uintptr_t up = get_user_page();

  assert((boot_pml4[bit_select(48, 39, up)] & ~0xFFF) == (uintptr_t)boot_pdpt, "PML4 is broken");

  boot_pdpt[bit_select(39, 30, up)] = (uintptr_t)user_pd | PTE_P | PTE_U;
  user_pd[bit_select(30, 21, up)] = (uintptr_t)user_pt | PTE_P | PTE_U;
  user_pt[bit_select(21, 12, up)] = (uintptr_t)get_user_page_backing() | PTE_P | PTE_U;

  // No TLB invalidation necessary, because we only created new entries. But we
  // need to make sure the compiler actually writes the values.
  asm volatile ("" ::: "memory");
}
