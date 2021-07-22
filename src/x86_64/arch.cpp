#include "arch.hpp"
#include "avx.hpp"
#include "entry.hpp"
#include "paging.hpp"
#include "selectors.hpp"
#include "x86.hpp"
#include "util.hpp"
#include "cpu_features.hpp"

extern "C" void irq_entry(exception_frame &);

// We only need interrupt descriptors for exceptions.
static idt_desc idt[irq_entry_count];
static tss tss;
static gdt_desc gdt[6] {
  {},
  gdt_desc::kern_code64_desc(),
  gdt_desc::kern_data64_desc(),
  gdt_desc::tss_desc(&tss),
  gdt_desc::user_code64_desc(),
  gdt_desc::user_data64_desc(),
};

// The RIP where execution continues after a user space exception.
static void *ring0_continuation = nullptr;

// The place where to store exception frames from user space.
static exception_frame *ring3_exception_frame = nullptr;

void setup_idt()
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
  format("!!! exception ", ef.vector, " (", hex(ef.error_code), ") at ",
	 hex(ef.cs), ":", hex(ef.ip), "\n");
  format("!!! CR2 ", hex(get_cr2()), "\n");
  format("!!! RDI ", hex(ef.rdi), "\n");
  format("!!! RSI ", hex(ef.rsi), "\n");
}

void irq_entry(exception_frame &ef)
{
  if ((ef.ss & 3) and ring0_continuation) {
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
exception_frame execute_user(uintptr_t rip)
{
  static uint64_t clobbered_rbp;
  exception_frame user {};

  user.cs = ring3_code_selector;
  user.ip = rip;
  user.ss = ring3_data_selector;
  user.rflags = (1 /* TF */ << 8) | 2;

  ring3_exception_frame = &user;

  // Prepare our stack to call irq_exit and exit to user space. We save a
  // continuation so we return here after an exception.
  //
  // TODO If we get a ring0 exception, we have a somewhat clobbered stack pointer.
  asm ("mov %%rbp, %[rbp_save]\n"
       "lea 1f, %%eax\n"
       "mov %%eax, %[cont]\n"
       "mov %%rsp, %[ring0_rsp]\n"
       "lea %[user], %%rsp\n"
       "jmp irq_exit\n"
       "1:\n"
       "mov %[rbp_save], %%rbp\n"
       : [ring0_rsp] "=m" (tss.rsp[0]), [cont] "=m" (ring0_continuation),
	 [user] "+m" (user), [rbp_save] "=m" (clobbered_rbp)
       :
       // Everything except RBP is clobbered, because we come back via irq_entry
       // after basically executing random bytes.
       : "rax", "rcx", "rdx", "rbx", "rsi", "rdi",
	 "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
	 "memory");

  return user;
}

extern "C" cpu_features const *setup_arch()
{
  setup_idt();
  setup_paging();
  try_setup_avx();

  static cpu_features features;

  // 64-bit x86 always has support for NX.
  features.has_nx = true;

  return &features;
}
