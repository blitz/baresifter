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

  for (size_t i = 0; i < irq_entry_count; i++) {
    idt[i] = idt_desc::interrupt_gate(ring0_code_selector,
                                      reinterpret_cast<uint64_t>(irq_entry_start + i*entry_fn_size),
                                      0, 0);
  }

  struct {
    uint16_t length = sizeof(gdt) - 1;
    void *gdtp = &gdt;
  } __attribute__((packed)) gdtr;

  // Load a new GDT that also includes a task gate.
  asm volatile ("lgdt %0" :: "m" (gdtr));

  // Our selectors are already correct, because we use the same ones as in the
  // boot GDT.

  // Point the task register to the newly created task gate, so the CPU knows
  // where to find stacks for exception/interrupt handling.
  asm volatile ("ltr %0" :: "r" ((uint16_t)ring0_tss_selector));

  struct {
    uint16_t length = sizeof(idt) - 1;
    void *idtp = idt;
  } __attribute__((packed)) idtr;

  asm volatile ("lidt %0" :: "m" (idtr));
}

void irq_entry(exception_frame &ef)
{
  print_string("!!! exception "); print_hex(ef.vector);
  print_string(" at "); print_hex(ef.cs); print_string(":"); print_hex(ef.rip);
  print_char('\n');

  // XXX Remove this.
  if (ef.vector == 6) {
    ef.rip = ef.rdi;
  } else {
    wait_forever();
  }
}
