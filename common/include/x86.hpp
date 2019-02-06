// x86_64 CPU data structures and helper functions

#pragma once

#include <cstddef>
#include <cstdint>

#ifdef __x86_64__
using mword_t = uint64_t;
#else
using mword_t = uint32_t;
#endif

// Constants

enum : mword_t {
  PTE_P = 1 << 0,
  PTE_W = 1 << 1,
  PTE_U = 1 << 2,
  PTE_PS = 1 << 7,
};

enum {
  CR4_PSE = 1 << 4,
};

// Task State Segment
struct tss {
#ifdef __x86_64__
  uint32_t reserved0;
  uint64_t rsp[3];
  uint32_t reserved1;
  uint32_t reserved2;
  uint64_t ist[7];
  uint32_t reserved3;
  uint32_t reserved4;
  uint16_t reserved5;
  uint16_t iopb_offset;
#else
  uint16_t prev_task;
  uint16_t reserved0;
  uint32_t esp0;
  uint32_t ss0;

  uint32_t esp1;
  uint32_t ss1;

  uint32_t esp2;
  uint32_t ss2;

  uint32_t esp3;
  uint32_t ss3;

  uint32_t cr3;

  uint32_t eip;
  uint32_t eflags;

  uint32_t gpr[8];

  uint32_t es;
  uint32_t cs;
  uint32_t ss;
  uint32_t ds;
  uint32_t fs;
  uint32_t gs;

  uint32_t ldt;

  uint32_t flags_iobm;
#endif
} __attribute__((packed));

// A descriptor for the Global Descriptor Table
struct gdt_desc {
  uint16_t limit_lo = 0;
  uint16_t base_lo = 0;
  uint8_t  base_mid = 0;
  uint8_t  type_dpl = 0;
  uint8_t  limit_flags = 0;
  uint8_t  base_hi = 0;

#ifdef __x86_64__
  uint32_t base_hi2 = 0;
  uint32_t reserved = 0;
#endif

  gdt_desc() = default;

  void set_base(uintptr_t base)
  {
    base_lo = base & 0xFFFF;
    base_mid = (base >> 16) & 0xFF;
    base_hi = (base >> 24) & 0xFF;
#ifdef __x86_64__
    base_hi2 = base >> 32;
#endif
  }

  // Set the maximum possible limit for the segment descriptor. What this means
  // depends on the G (granularity) bit.
  void set_max_limit()
  {
    limit_lo = 0xFFFF;
    limit_flags |= 0xF;
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

  // A flat 64-bit CPL0 code segment.
  static gdt_desc kern_code64_desc()
  {
    gdt_desc t;
    t.type_dpl = 0b10011011;
    t.limit_flags = 0b10100000;
    return t;
  }

  // A flat 32-bit CPL0 code segment.
  static gdt_desc kern_code32_desc()
  {
    gdt_desc t;
    t.type_dpl = 0b10011011;
    t.limit_flags = 0b11000000;
    t.set_max_limit();
    return t;
  }

  // A flat 64-bit CPL0 data segment.
  static gdt_desc kern_data64_desc()
  {
    gdt_desc t;
    t.type_dpl = 0b10010011;
    t.limit_flags = 0b10100000;
    return t;
  }

  // A flat 32-bit CPL0 data segment.
  static gdt_desc kern_data32_desc()
  {
    gdt_desc t;
    t.type_dpl = 0b10010011;
    t.limit_flags = 0b11000000;
    t.set_max_limit();
    return t;
  }

  // A flat 32-bit CPL0 code segment.
  static gdt_desc user_code64_desc()
  {
    gdt_desc t;
    t.type_dpl = 0b11111011;
    t.limit_flags = 0b10100000;
    return t;
  }

  // A flat 32-bit CPL3 code segment.
  static gdt_desc user_code32_desc()
  {
    gdt_desc t;
    t.type_dpl = 0b11111011;
    t.limit_flags = 0b11000000;
    t.set_max_limit();
    return t;
  }

  // A flat 64-bit CPL3 data segment.
  static gdt_desc user_data64_desc()
  {
    gdt_desc t;
    t.type_dpl = 0b11110011;
    t.limit_flags = 0b10100000;
    return t;
  }

  // A flat 32-bit CPL3 data segment.
  static gdt_desc user_data32_desc()
  {
    gdt_desc t;
    t.type_dpl = 0b11110011;
    t.limit_flags = 0b11000000;
    t.set_max_limit();
    return t;
  }

};

#ifdef __x86_64__
static_assert(sizeof(gdt_desc) == 16, "GDT structure broken");
#else
static_assert(sizeof(gdt_desc) == 8, "GDT structure broken");
#endif

// Interrupt descriptor table descriptor
struct idt_desc {
  uint16_t offset_lo = 0;
  uint16_t selector = 0;
  uint16_t flags = 0;
  uint16_t offset_hi = 0;
#ifdef __x86_64__
  uint32_t offset_hi2 = 0;
  uint32_t reserved = 0;
#endif

  idt_desc() = default;

  static idt_desc interrupt_gate(uint16_t sel_, uintptr_t offset_, uint8_t ist, uint8_t dpl)
  {
    idt_desc i;

    i.offset_lo = offset_ & 0xFFFF;
    i.selector = sel_;
    i.flags = ist | (14 /* type */ << 8) | (dpl << 13) | (1 << 15 /* Present */);
    i.offset_hi = (offset_ >> 16) & 0xFFFF;
#ifdef __x86_64__
    i.offset_hi2 = offset_ >> 32;
#endif

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

// Generic 8-bit IN operation
inline uint8_t inb(uint16_t port)
{
  uint8_t v;
  asm volatile ("inb (%%dx), %%al" : "=a" (v) : "d" (port));
  return v;
}

inline void set_cr0(mword_t v) { asm volatile ("mov %0, %%cr0" :: "r" (v)); }
inline void set_cr2(mword_t v) { asm volatile ("mov %0, %%cr2" :: "r" (v)); }
inline void set_cr3(mword_t v) { asm volatile ("mov %0, %%cr3" :: "r" (v) : "memory"); }
inline void set_cr4(mword_t v) { asm volatile ("mov %0, %%cr4" :: "r" (v)); }

inline mword_t get_cr0() { mword_t v; asm volatile ("mov %%cr0, %0" : "=r" (v)); return v; }
inline mword_t get_cr2() { mword_t v; asm volatile ("mov %%cr2, %0" : "=r" (v)); return v; }
inline mword_t get_cr3() { mword_t v; asm volatile ("mov %%cr3, %0" : "=r" (v)); return v; }
inline mword_t get_cr4() { mword_t v; asm volatile ("mov %%cr4, %0" : "=r" (v)); return v; }

inline void set_xcr0(uint64_t v)
{
  asm volatile ("xsetbv" ::
                "d" ((uint32_t)(v >> 32)), "a" ((uint32_t)v), "c" (0));
}

inline uint64_t get_xcr0()
{
  uint32_t lo, hi;
  asm volatile ("xgetbv" : "=d" (hi), "=a" (lo) : "c" (0));
  return (uint64_t)hi << 32 | lo;
}

inline uint64_t rdtsc()
{
  uint32_t hi, lo;
  asm volatile ("rdtsc" : "=a" (lo), "=d" (hi));
  return (uint64_t)hi << 32 | lo;
}

inline void pause()
{
  asm volatile ("pause");
}
