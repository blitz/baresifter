#include <cstdint>

#include "paging.hpp"

static const uint64_t PTE_P = 1 << 0;
static const uint64_t PTE_U = 1 << 2;

extern char _image_end[];

char *get_user_page()
{
  return _image_end;
}

// These are our boot page table structures, which are partly setup by the
// assembler startup code.
extern "C" uint64_t boot_pml4[512];
extern "C" uint64_t boot_pdpt[512];
extern "C" uint64_t boot_pd[512];

#define page_align alignas(4096)

page_align uint64_t boot_pml4[512];
page_align uint64_t boot_pdpt[512]; // Covers 0 - 512GB
page_align uint64_t boot_pd[512];   // Covers 0 - 1GB

page_align static uint64_t user_pt[512];

static uint64_t bit_select(int high, int low, uint64_t value)
{
  return (value >> low) & ((1UL << (high - low)) - 1);
}

void setup_paging()
{
  boot_pd[bit_select(30, 21, (uintptr_t)get_user_page())] = (uintptr_t)user_pt | PTE_P | PTE_U;
  user_pt[bit_select(21, 12, (uintptr_t)get_user_page())] = (uintptr_t)get_user_page() | PTE_P | PTE_U;

  // No TLB invalidation necessary, because we only created new entries. But we
  // need to make sure the compiler actually writes the values.
  asm volatile ("" ::: "memory");
}
