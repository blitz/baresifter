#include <cstdint>

#include "entry.hpp"
#include "idt.hpp"
#include "io.hpp"
#include "x86.hpp"

struct gdt_desc {
  uint16_t limit_lo = 0;
  uint16_t base_lo = 0;
  uint8_t  base_mid = 0;
  uint8_t  type_dpl = 0;
  uint8_t  limit_flags = 0;
  uint8_t  base_hi = 0;
  uint32_t base_hi2 = 0;
  uint32_t reserved = 0;

  gdt_desc() = default;

  void set_base(uint64_t base)
  {
    base_lo = base & 0xFFFF;
    base_mid = (base >> 16) & 0xFF;
    base_hi = (base >> 24) & 0xFF;
    base_hi2 = base >> 32;
  }

  static gdt_desc tss_desc(uint32_t limit, uint64_t base)
  {
    gdt_desc t;

    assert(limit < 0x10000, "Limit must fit into 16-bit");

    t.limit_lo = limit & 0xFFFF;
    t.type_dpl = 0b10001001;
    t.set_base(base);
    return t;
  }

  static gdt_desc kern_code_desc()
  {
    gdt_desc t;
    t.type_dpl = (1 << 7 /* P */) | 0b11011;
    t.limit_flags = 0b1010 << 4;
    return t;
  }

  static gdt_desc kern_data_desc()
  {
    gdt_desc t;
    t.type_dpl = (1 << 7 /* P */) | 0b10011;
    t.limit_flags = 0b1010 << 4;
    return t;
  }
};

static_assert(sizeof(gdt_desc) == 16, "GDT structure broken");

struct idt_desc {
  uint16_t offset_lo;
  uint16_t selector;
  uint16_t flags;
  uint16_t offset_hi;
  uint32_t offset_hi2;
  uint32_t reserved = 0;

  idt_desc() = default;

  idt_desc(uint16_t sel_, uint64_t offset_, uint8_t ist, uint8_t type, uint8_t dpl)
    : offset_lo(offset_ & 0xFFFF),
      selector(sel_),
      flags(ist | (type << 8) | (dpl << 13) | (1 << 15 /* Present */)),
      offset_hi((offset_ >> 16) & 0xFFFF),
      offset_hi2(offset_ >> 32)
  {}
};

struct tss {
  uint32_t reserved0;
  uint64_t rsp[3];
  uint32_t reserved1;
  uint32_t reserved2;
  uint64_t ist[7];
  uint32_t reserved3;
  uint32_t reserved4;
  uint16_t reserved5;
  uint16_t iopb_offset;
} __attribute__((packed));

// We only need interrupt descriptors for exceptions.
static idt_desc idt[irq_entry_count];
static tss tss;
static gdt_desc gdt[4] {
  {},
  gdt_desc::kern_code_desc(),
  gdt_desc::kern_data_desc(),
  gdt_desc::tss_desc(sizeof(tss) - 1, reinterpret_cast<uint64_t>(&tss)),
};

void setup_idt()
{
  size_t entry_fn_size = (irq_entry_end - irq_entry_start) / irq_entry_count;

  for (size_t i = 0; i < irq_entry_count; i++) {
    idt[i] = idt_desc { ring0_code_selector,
                        reinterpret_cast<uint64_t>(irq_entry_start + i*entry_fn_size),
                        0, 14 /* Interrupt gate */, 0 };
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
