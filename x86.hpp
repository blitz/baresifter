// x86_64 CPU data structures and helper functions

#pragma once

#include <cstddef>
#include <cstdint>

// Task State Segment
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

// A descriptor for the Global Descriptor Table
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

  static gdt_desc tss_desc(tss const *base)
  {
    gdt_desc t;

    static_assert(sizeof(tss) < 0x10000U, "TSS size broken");

    t.limit_lo = sizeof(tss) & 0xFFFF;
    t.type_dpl = 0b10001001;
    t.set_base((uintptr_t)base);
    return t;
  }

  static gdt_desc kern_code_desc()
  {
    gdt_desc t;
    t.type_dpl = 0b10011011;
    t.limit_flags = 0b10100000;
    return t;
  }

  static gdt_desc kern_data_desc()
  {
    gdt_desc t;
    t.type_dpl = 0b10010011;
    t.limit_flags = 0b10100000;
    return t;
  }

  static gdt_desc user_code_desc()
  {
    gdt_desc t;
    t.type_dpl = 0b11111011;
    t.limit_flags = 0b10100000;
    return t;
  }

  static gdt_desc user_data_desc()
  {
    gdt_desc t;
    t.type_dpl = 0b11110011;
    t.limit_flags = 0b10100000;
    return t;
  }
};

static_assert(sizeof(gdt_desc) == 16, "GDT structure broken");

// Interrupt descriptor table descriptor
struct idt_desc {
  uint16_t offset_lo = 0;
  uint16_t selector = 0;
  uint16_t flags = 0;
  uint16_t offset_hi = 0;
  uint32_t offset_hi2 = 0;
  uint32_t reserved = 0;

  idt_desc() = default;

  static idt_desc interrupt_gate(uint16_t sel_, uint64_t offset_, uint8_t ist, uint8_t dpl)
  {
    idt_desc i;

    i.offset_lo = offset_ & 0xFFFF;
    i.selector = sel_;
    i.flags = ist | (14 /* type */ << 8) | (dpl << 13) | (1 << 15 /* Present */);
    i.offset_hi = (offset_ >> 16) & 0xFFFF;
    i.offset_hi2 = offset_ >> 32;

    return i;
  }
};

// Descriptor table register (GDTR/LDTR)
template <typename DESC>
struct dtr {
  uint16_t length;
  DESC const *descp;

  template <size_t N>
  dtr(DESC (&descp_)[N])
    : length(N * sizeof(DESC) - 1),
      descp(descp_)
  {}
} __attribute__((packed));

inline void lidt(dtr<idt_desc> const &idtr)
{
  asm volatile ("lidt %0" :: "m" (idtr));
}

inline void lgdt(dtr<gdt_desc> const &gdtr)
{
  asm volatile ("lgdt %0" :: "m" (gdtr));
}

inline void ltr(uint16_t selector)
{
  asm volatile ("ltr %0" :: "r" ((uint16_t)selector));
}

// Immediate 8-bit OUT operation
template <int PORT>
inline void outbi(uint8_t data)
{
  static_assert(PORT >= 0 and PORT < 0x100, "Port is out of range");
  asm volatile ("outb %%al, %0" :: "N" (PORT), "a" (data));
}

// Generic 8-bit OUT operation
inline void outb(uint16_t port, uint8_t data)
{
  asm volatile ("outb %%al, (%%dx)" :: "d" (port), "a" (data));
}

inline uint64_t get_cr2()
{
  uint64_t v;
  asm ("mov %%cr2, %0" : "=r" (v));
  return v;
}

inline uint64_t rdtsc()
{
  uint32_t hi, lo;
  asm volatile ("rdtsc" : "=a" (lo), "=d" (hi));
  return (uint64_t)hi << 32 | lo;
}
