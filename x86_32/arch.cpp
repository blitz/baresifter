#include "arch.hpp"
#include "entry.hpp"
#include "selectors.hpp"
#include "util.hpp"
#include "x86.hpp"

extern "C" void irq_entry(exception_frame &);

extern "C" char _image_start[];
extern "C" char _image_end[];

// Page directory. Kernel code is covered using 2MB entries here.
alignas(page_size) static uint32_t pdt[page_size / sizeof(uint32_t)];

// Page table for user code.
alignas(page_size) static uint32_t user_pt[page_size / sizeof(uint32_t)];

alignas(page_size) static char user_page_backing[page_size];

static tss tss;

// The RIP where execution continues after a user space exception.
static void *ring0_continuation = nullptr;

// The place where to store exception frames from user space.
static exception_frame *ring3_exception_frame = nullptr;

char *get_user_page_backing()
{
  return user_page_backing;
}

uintptr_t get_user_page()
{
  // Needs to be in the reach of 16-bit code, so we don't need a different
  // mapping for 16-bit code. Don't use 1MB directly, because this will case the
  // sifting algorithm to find accidentally generate valid memory addresses and
  // needlessly enlarge the search space.

  return (1UL << 20 /* MiB */) + page_size;
}

static bool is_aligned(uint64_t v, int order)
{
  assert(order < sizeof(v)*8, "Order out of range");
  return 0 == (v & ((uint64_t(1) << order) - 1));
}

static void setup_paging()
{
  uintptr_t istart = reinterpret_cast<uintptr_t>(_image_start);
  uintptr_t iend = reinterpret_cast<uintptr_t>(_image_end);

  assert(is_aligned(istart, 22), "Image needs to start on large page boundary");

  // Map our binary 1:1
  for (uintptr_t c = istart; c <= iend; c += (1U << 22)) {
    uintptr_t idx = c >> 22;
    pdt[idx] = c | PTE_P | PTE_W | PTE_PS;
  }

  // Map user page
  uintptr_t up = get_user_page();

  assert(up + page_size <= istart, "User page cannot be mapped into kernel area");
  pdt[bit_select(32, 22, up)] = reinterpret_cast<uintptr_t>(user_pt) | PTE_U | PTE_P;
  user_pt[bit_select(22, 12, up)] = reinterpret_cast<uintptr_t>(get_user_page_backing()) | PTE_U | PTE_P;

  set_cr4(get_cr4() | CR4_PSE | CR4_SMEP);
  set_cr3((uintptr_t)pdt);
  set_cr0(get_cr0() | CR0_PG | CR0_WP);
}

static void setup_gdt()
{
  static gdt_desc gdt[] {
    {},
    gdt_desc::kern_code32_desc(),
    gdt_desc::kern_data32_desc(),
    gdt_desc::tss_desc(&tss),
    gdt_desc::user_code32_desc(),
    gdt_desc::user_data32_desc(),
  };

  lgdt(gdt);

  tss.ss0 = ring0_data_selector;
  ltr(ring0_tss_selector);

  // Reload code segment descriptor
  asm volatile ("ljmp %[selector], $1f\n"
                "1:\n"
                :: [selector] "i" (static_cast<uint32_t>(ring0_code_selector)));

  // Reload data segment descriptors. We load user selectors for everything
  // except SS to avoid having to reload them.
  asm volatile ("mov %0, %%ss\n"
                "mov %1, %%ds\n"
                "mov %1, %%es\n"
                "mov %1, %%fs\n"
                "mov %1, %%gs\n"
                :: "r" (ring0_data_selector), "r" (ring3_data_selector));

  // XXX Setup code/stack segments for 32-bit and 16-bit protected mode.
}

static void print_exception(exception_frame const &ef)
{
  format("!!! exception ", ef.vector, " (", hex(ef.error_code), ") at ",
         hex(ef.cs), ":", hex(ef.ip), "\n");
  format("!!! CR2 ", hex(get_cr2()), "\n");
  format("!!! EDI ", hex(ef.edi), "\n");
  format("!!! ESI ", hex(ef.esi), "\n");
}

void irq_entry(exception_frame &ef)
{
  // We have to check CS here, because for kernel exceptions SS is not pushed.
  if ((ef.cs & 3) and ring0_continuation) {
    auto ret = ring0_continuation;
    ring0_continuation = nullptr;

    if (ring3_exception_frame) {
      *ring3_exception_frame = ef;
      ring3_exception_frame = nullptr;
    }

    asm ("mov %0, %%esp\n"
         "jmp *%1\n"
         :: "r" (tss.esp0), "r" (ret) : "memory");
    __builtin_unreachable();
  }

  print_exception(ef);
  format("!!! We're dead...\n");
  wait_forever();
}

// Execute user code at the specified address. Returns after an exception with
// the details of the exception.
exception_frame execute_user(uintptr_t ip)
{
  static uint32_t clobbered_ebp;
  exception_frame user {};

  user.cs = ring3_code_selector;
  user.ip = ip;
  user.ss = ring3_data_selector;
  user.eflags = (1 /* TF */ << 8) | 2;

  ring3_exception_frame = &user;

  // Prepare our stack to call irq_exit and exit to user space. We save a
  // continuation so we return here after an exception.
  //
  // TODO If we get a ring0 exception, we have a somewhat clobbered stack pointer.
  asm ("mov %%ebp, %[ebp_save]\n"
       "lea 1f, %%eax\n"
       "mov %%eax, %[cont]\n"
       "mov %%esp, %[ring0_rsp]\n"
       "lea %[user], %%esp\n"
       "jmp irq_exit\n"
       "1:\n"
       "mov %[ebp_save], %%ebp\n"
       : [ring0_rsp] "=m" (tss.esp0), [cont] "=m" (ring0_continuation),
         [user] "+m" (user), [ebp_save] "=m" (clobbered_ebp)
       :
       // Everything except EBP is clobbered, because we come back via irq_entry
       // after basically executing random bytes.
       : "eax", "ecx", "edx", "ebx", "esi", "edi",
         "memory");

  return user;
}

static void setup_idt()
{
  static idt_desc idt[irq_entry_count];
  size_t entry_fn_size = (irq_entry_end - irq_entry_start) / irq_entry_count;

  for (size_t i = 0; i < array_size(idt); i++) {
    idt[i] = idt_desc::interrupt_gate(ring0_code_selector,
                                      reinterpret_cast<uint64_t>(irq_entry_start + i*entry_fn_size),
                                      0, 0);
  }

  lidt(idt);
}

void setup_arch()
{
  setup_paging();
  setup_gdt();
  setup_idt();
}
