#include "arch.hpp"
#include "util.hpp"

static const uint64_t PTE_P = 1 << 0;
static const uint64_t PTE_U = 1 << 2;

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

#define page_align alignas(4096)

page_align uint64_t boot_pml4[512];
page_align uint64_t boot_pdpt[512]; // Covers 0 - 512GB
page_align uint64_t boot_pd[512];   // Covers 0 - 1GB

page_align static uint64_t user_pd[512]; // Covers 4GB-5GB
page_align static uint64_t user_pt[512]; // Covers 4GB to 4GB+4K

page_align static char user_page_backing[page_size];

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

  assert((boot_pml4[bit_select(48, 49, up)] & ~0xFFF) == (uintptr_t)boot_pdpt, "PML4 is broken");

  boot_pdpt[bit_select(39, 30, up)] = (uintptr_t)user_pd | PTE_P | PTE_U;
  user_pd[bit_select(30, 21, up)] = (uintptr_t)user_pt | PTE_P | PTE_U;
  user_pt[bit_select(21, 12, up)] = (uintptr_t)get_user_page_backing() | PTE_P | PTE_U;

  // No TLB invalidation necessary, because we only created new entries. But we
  // need to make sure the compiler actually writes the values.
  asm volatile ("" ::: "memory");
}
