#include <cstdint>
#include <cstring>
#include <initializer_list>

#include "disassemble.hpp"
#include "entry.hpp"
#include "exception_frame.hpp"
#include "logo.hpp"
#include "paging.hpp"
#include "selectors.hpp"
#include "util.hpp"
#include "x86.hpp"

struct execution_attempt {
  uint8_t length = 0;
  uint8_t exception = 0;
};

bool operator==(execution_attempt const &a, execution_attempt const &b)
{
  return a.length == b.length and a.exception == b.exception;
}

bool operator!=(execution_attempt const &a, execution_attempt const &b)
{
  return not (a == b);
}

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
  format("!!! exception ", ef.vector, " (", hex(ef.error_code), ") at ",
         hex(ef.cs), ":", hex(ef.rip), "\n");
  format("!!! CR2 ", hex(get_cr2()), "\n");
  format("!!! EDI ", hex(ef.rdi), "\n");
  format("!!! ESI ", hex(ef.rsi), "\n");
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

static execution_attempt find_instruction_length(instruction_bytes const &instr)
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
      return { (uint8_t)i, (uint8_t)ef.vector };
  }

  format("Could not find instruction length?\n");
  wait_forever();
}

static void self_test_instruction_length()
{
  struct {
    size_t length;
    instruction_bytes instr;
  } tests[] {
    { 1, { 0x90 } },            // nop
    { 1, { 0xCC } },            // int3
    { 2, { 0xCD, 0x01 } },      // int 0x01
    { 2, { 0x00, 0x00 } },      // add [rax], al
    { 2, { 0xEB, 00 } },        // jmp 0x2
    { 5, { 0xE9, 0x00, 0x00, 0x00, 0x00 } }, // jmp 0x5
    { 6, { 0x66, 0xE9, 0x00, 0x00, 0x00, 0x00 } }, // jmp 0x6
    // lock add qword cs:[eax+4*eax+07e06df23h], 0efcdab89h
    { 15, { 0x2e, 0x67, 0xf0, 0x48,
            0x81, 0x84, 0x80, 0x23,
            0xdf, 0x06, 0x7e, 0x89,
            0xab, 0xcd, 0xef }},
  };

  bool success = true;

  for (auto const &test : tests) {
    auto attempt = find_instruction_length(test.instr);
    if (attempt.length != test.length)
      success = false;
  }

  format("Instruction length: ", (success ? "OK" : "b0rken!"), "\n");
  if (not success) {
    wait_forever();
  }
}

static void print_instruction(instruction_bytes const &instr,
                              execution_attempt const &attempt)
{
  // Prefix instruction, so it's easy to grep output.
  format("E ", hex(attempt.exception, 2, false), " O ");

  auto disasm = disassemble(instr);
  format(hex(disasm.instruction, 4, false), " ");

  if (disasm.instruction != X86_INS_INVALID and disasm.length != attempt.length) {
    // The CPU and Capstone successfully decoded an instruction, but they
    // disagree about the length. This is likely a Capstone bug.
    format("BUG ");
  } else if (attempt.exception != 6 and disasm.instruction == X86_INS_INVALID) {
    // The CPU successfully executed an instruction (i.e. #UD exception), but
    // capstone is clueless what that is.
    format("UNKN");
  } else {
    format("OK  ");
  }

  format(" |");
  for (size_t i = 0; i < attempt.length; i++) {
    format(" ", hex(instr.raw[i], 2, false));
  }

  format("\n");
}

static bool is_interesting_change(execution_attempt const &last,
                                  execution_attempt const &now)
{
  if (last == now)
    return false;

  // This is a special case for FXSAVE, which has an alignment restriction on
  // its memory operand. It will flip between #GP and #PF depending on the
  // memory operand. Mark this case as non-interesting.
  if (last.length == now.length and
      ((last.exception == 0xE and now.exception == 0xD) or
       (last.exception == 0xD and now.exception == 0xE)))
    return false;

  return true;
}

void start()
{
  print_logo();

  format(">>> Setting up IDT.\n");
  setup_idt();

  format(">>> Setting up paging.\n");
  setup_paging();

  format(">>> Setting up Capstone.\n");
  setup_disassembler();

  format(">>> Executing self test.\n");
  self_test_instruction_length();
  disassemble_self_test();

  format(">>> Probing instruction space.\n");

  instruction_bytes current;
  execution_attempt last_attempt;
  size_t inc_pos = 0;

  while (true) {
    auto attempt = find_instruction_length(current);

    // Clear unused bytes.
    memset(current.raw + attempt.length, 0, sizeof(current.raw) - attempt.length);

    // Something interesting has happened, start searching from the end again.
    if (is_interesting_change(last_attempt, attempt)) {
      inc_pos = attempt.length - 1;
      print_instruction(current, attempt);
    }

  again:
    current.raw[inc_pos]++;
    if (current.raw[inc_pos] == 0) {
      // We've wrapped at our current position, so go left one byte. If we hit
      // the beginning, we are done.
      if (inc_pos-- == 0) {
        break;
      }

      goto again;
    }

    last_attempt = attempt;
  }

  format(">>> Done!\n");
}
