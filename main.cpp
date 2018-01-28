#include <cstdint>
#include <initializer_list>

#include "entry.hpp"
#include "exception_frame.hpp"
#include "logo.hpp"
#include "paging.hpp"
#include "selectors.hpp"
#include "stdlib.hpp"
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
  user.rflags = (1 /* TF */ << 8) | 2;

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

struct instruction {
  // x86 instructions are at most 15 bytes long.
  uint8_t raw[15];

  template <typename... T>
  constexpr instruction(T... v)
  : raw {(uint8_t)v...}
  {}
};

static size_t find_instruction_length(instruction const &instr)
{
  char * const user_page = get_user_page();

  for (size_t i = 1; i <= array_size(instr.raw); i++) {
    char * const instr_start = user_page + page_size - i;
    memcpy(instr_start, instr.raw, i);

    auto ef = execute_user(instr_start);

    bool incomplete_instruction_fetch = (ef.vector == 14 and
                                         (ef.error_code & 0b10101 /* user space instruction fetch */) == 0b10100 and
                                         get_cr2() == (uintptr_t)user_page + page_size);

    if (not incomplete_instruction_fetch)
      return i;
  }

  format("Could not find instruction length?\n");
  wait_forever();
}

static void self_test_instruction_length()
{
  struct {
    size_t length;
    instruction instr;
  } tests[] {
    { 1, { 0x90 } },            // nop
    { 1, { 0xCC } },            // int3
    { 2, { 0xCD, 0x01 } },      // int 0x01
    { 2, { 0x00, 0x00 } },      // add [rax], al

    // lock add qword cs:[eax+4*eax+07e06df23h], 0efcdab89h
    { 15, { 0x2e, 0x67, 0xf0, 0x48,
            0x81, 0x84, 0x80, 0x23,
            0xdf, 0x06, 0x7e, 0x89,
            0xab, 0xcd, 0xef }},
  };

  bool success = true;

  for (auto const &test : tests) {
    size_t len = find_instruction_length(test.instr);
    if (len != test.length)
      success = false;
  }

  format("Self test: ", (success ? "OK" : "b0rken!"), "\n");
}

static void print_instruction(instruction const &instr, size_t length)
{
  // Prefix instruction, so it's easy to grep output.
  format("I");

  for (size_t i = 0; i < length; i++) {
    format(" ", hex(instr.raw[i], 2, false));
  }

  format("\n");
}

void start()
{
  print_logo();

  format(">>> Setting up IDT.\n");
  setup_idt();

  format(">>> Setting up paging.\n");
  setup_paging();

  format(">>> Executing self test.\n");
  self_test_instruction_length();

  format(">>> Probing instruction space.\n");

  instruction current;
  size_t last_length = 0;
  size_t inc_pos = 0;

  while (true) {
    size_t length = find_instruction_length(current);

    // Clear unused bytes.
    memset(current.raw + length, 0, sizeof(current.raw) - length);

    // Something interesting has happened, start searching from the end again.
    if (last_length != length) {
      //format("Length difference! Incrementing at ", length - 1, "\n");
      inc_pos = length - 1;

      print_instruction(current, length);
    }

    current.raw[inc_pos]++;
    if (current.raw[inc_pos] == 0) {
      // We've wrapped at our current position, so go left one byte. If we hit
      // the beginning, we are done.
      if (inc_pos-- == 0) {
        break;
      }

      current.raw[inc_pos]++;
    }

    last_length = length;
  }

  format(">>> Done!\n");
}
