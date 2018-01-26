#include <cstdint>

#include "entry.hpp"
#include "exception_frame.hpp"
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

void irq_entry(exception_frame &ef)
{
  if (unlikely(ef.cs & 0x3)) {
    format("!!! ring0 exception ", ef.vector, " (", ef.error_code, ") at ", ef.cs, ":", ef.rip, "\n");
    switch (ef.vector) {
    case 14:
      format("!!! CR2 ", get_cr2(), "\n");
      break;
    }

    format("!!! We're dead...\n");
    wait_forever();
  }

  format("??? User space fault?\n");
  wait_forever();
}

void start()
{
  format(">>> Executing constructors.\n");
  execute_constructors();

  format(">>> Setting up IDT.\n");
  setup_idt();

  format(">>> Setting up paging.\n");
  setup_paging();

  format(">>> Done!\n");
}
