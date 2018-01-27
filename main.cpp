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

// The RIP where execution continues after a user space exception.
static void *ring0_continuation = nullptr;

// The place where to store exception frames from user space.
static exception_frame *ring3_exception_frame = nullptr;

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

static void print_exception(exception_frame const &ef)
{
  format("!!! exception ", ef.vector, " (", ef.error_code, ") at ", ef.cs, ":", ef.rip, "\n");
  format("!!! CR2 ", get_cr2(), "\n");
}

void irq_entry(exception_frame &ef)
{
  if ((ef.cs & 3) and ring0_continuation) {
    auto ret = ring0_continuation;
    ring0_continuation = nullptr;

    if (ring3_exception_frame) {
      *ring3_exception_frame = ef;
      ring3_exception_frame = nullptr;
    }

    asm ("mov %0, %%rsp\n"
         "jmp *%1\n"
         :: "r" (tss.rsp[0]), "r" (ret) : "memory");
    __builtin_unreachable();
  }

  print_exception(ef);
  format("!!! We're dead...\n");
  wait_forever();
}

// Execute user code at the specified address. Returns after an exception with
// the details of the exception.
static exception_frame execute_user(void const *rip)
{
  exception_frame user {};

  user.cs = ring3_code_selector;
  user.rip = (uintptr_t)rip;
  user.ss = ring3_data_selector;
  user.rflags = 2;

  ring3_exception_frame = &user;

  // Prepare our stack to call irq_exit and exit to user space. We save a
  // continuation so we return here after an exception.
  //
  // TODO Use sysexit instead of iret to exit to user space faster.
  // TODO If we get a ring0 exception, we have a somewhat clobbered stack pointer.
  asm ("lea 1f, %%eax\n"
       "mov %%eax, %1\n"
       "mov %%rsp, %0\n"
       "lea %2, %%rsp\n"
       "jmp irq_exit\n"
       "1:\n" : "=m" (tss.rsp[0]), "=m" (ring0_continuation) : "m" (user)
       // Everything is clobbered, because we come back via irq_entry.
       : "rax", "rcx", "rdx", "rbx", "rsi", "rdi",
         "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15", "rbp",
         "memory");

  return user;
}

void start()
{
  print_logo();

  format(">>> Setting up IDT.\n");
  setup_idt();

  format(">>> Setting up paging.\n");
  setup_paging();

  format(">>> Executing user code.\n");

  uint64_t begin, end;
  const size_t repetitions = 32 * 1024;

  begin = rdtsc();
  for (size_t i = 0; i < repetitions; i++)
    execute_user(get_user_page());
  end = rdtsc();

  format(">>> Ticks per user space fault (very rough!): ", (end - begin) / repetitions, "\n");

  format(">>> User space exception: ", execute_user(get_user_page() + 4096 - 2).vector, "\n");

  format(">>> Done!\n");
}
