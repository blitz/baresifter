#include <cstdint>

#include "entry.hpp"
#include "exception_frame.hpp"
#include "logo.hpp"
#include "paging.hpp"
#include "selectors.hpp"
#include "util.hpp"
#include "x86.hpp"

extern "C" void irq_entry(exception_frame &);
extern "C" void start();

// We only need interrupt descriptors for exceptions.
static idt_desc idt[irq_entry_count];
static tss tss;
static gdt_desc gdt[6] {
  {},
  gdt_desc::kern_code_desc(),
  gdt_desc::kern_data_desc(),
  gdt_desc::tss_desc(&tss),
  gdt_desc::user_code_desc(),
  gdt_desc::user_data_desc(),
};

static void setup_idt()
{
  size_t entry_fn_size = (irq_entry_end - irq_entry_start) / irq_entry_count;

  for (size_t i = 0; i < array_size(idt); i++) {
    idt[i] = idt_desc::interrupt_gate(ring0_code_selector,
                                      reinterpret_cast<uint64_t>(irq_entry_start + i*entry_fn_size),
                                      0, 0);
  }

  // Load a new GDT that also includes a task gate.
  lgdt(gdt);

  // Our selectors are already correct, because we use the same ones as in the
  // boot GDT.

  // Point the task register to the newly created task gate, so the CPU knows
  // where to find stacks for exception/interrupt handling.
  ltr(ring0_tss_selector);

  lidt(idt);
}

static void *ring0_continuation = nullptr;

void irq_entry(exception_frame &ef)
{
  format("!!! exception ", ef.vector, " (", ef.error_code, ") at ", ef.cs, ":", ef.rip, "\n");
  format("!!! CR2 ", get_cr2(), "\n");

  if ((ef.cs & 3) and ring0_continuation) {
    format("!!! User space fault: returning to ", (uintptr_t)ring0_continuation, "\n");
    auto ret = ring0_continuation;
    ring0_continuation = nullptr;

    asm ("mov %0, %%rsp\n"
         "jmp *%1\n"
         :: "r" (tss.rsp[0]), "r" (ret));
    __builtin_unreachable();
  }

  format("!!! We're dead...\n");
  wait_forever();
}

static void execute_user(void const *rip)
{
  exception_frame user {};

  user.cs = ring3_code_selector;
  user.rip = (uintptr_t)rip;
  user.ss = ring3_data_selector;
  user.rflags = 2;

  // Prepare our stack to call irq_exit and exit to user space. We save a
  // continuation so we return here after an exception.
  asm ("lea 1f, %%eax\n"
       "mov %%eax, %1\n"
       "mov %%rsp, %0\n"
       "lea %2, %%rsp\n"
       "jmp irq_exit\n"
       "1:\n" : "=m" (tss.rsp[0]), "=m" (ring0_continuation) : "m" (user)
       // Everything is clobbered, because we come back via irq_entry.
       : "rax", "rcx", "rdx", "rbx", "rsi", "rdi",
         "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15", "rbp");
}

void start()
{
  print_logo();

  format(">>> Setting up IDT.\n");
  setup_idt();

  format(">>> Setting up paging.\n");
  setup_paging();

  format(">>> Executing user code.\n");

  char *u = get_user_page();
  u[0] = 0x31; u[1] = 0xc0;     // xor eax, eax
  u[2] = 0x8e; u[3] = 0xd8;     // mov eax, ds
  u[4] = 0x0f; u[5] = 0x30;     // wrmsr
  execute_user(get_user_page() + 0);

  execute_user(get_user_page() + 4096 - 2);

  format(">>> Done!\n");
}
