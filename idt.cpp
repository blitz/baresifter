#include <cstdint>

#include "entry.hpp"
#include "idt.hpp"
#include "io.hpp"
#include "selectors.hpp"
#include "x86.hpp"
#include "util.hpp"

// We only need interrupt descriptors for exceptions.
static idt_desc idt[irq_entry_count];
static tss tss;
static gdt_desc gdt[4] {
  {},
  gdt_desc::kern_code_desc(),
  gdt_desc::kern_data_desc(),
  gdt_desc::tss_desc(&tss),
};

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

void irq_entry(exception_frame &ef)
{
  format("!!! exception ", ef.vector, " at ", ef.cs, ":", ef.rip, "\n");

  // XXX Remove this.
  if (ef.vector == 6) {
    ef.rip = ef.rdi;
  } else {
    wait_forever();
  }
}
