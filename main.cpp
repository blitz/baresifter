#include <cstdint>
#include <cstring>
#include <cstdlib>

#include "arch.hpp"
#include "cpuid.hpp"
#include "disassemble.hpp"
#include "execution_attempt.hpp"
#include "logo.hpp"
#include "search.hpp"
#include "util.hpp"
#include "x86.hpp"

static execution_attempt find_instruction_length(instruction_bytes const &instr)
{
  exception_frame ef;
  size_t i;

  for (i = 1; i <= array_size(instr.raw); i++) {
    size_t const page_offset =  page_size - i;
    char * const instr_start = get_user_page_backing() + page_offset;
    uintptr_t const guest_ip = get_user_page() + page_offset;
    memcpy(instr_start, instr.raw, i);

    ef = execute_user(guest_ip);

    // The instruction hasn't been completely fetched, if we get an instruction
    // fetch page fault.
    //
    // The check for RIP is necessary, because otherwise we cannot correctly
    // guess the length of XBEGIN with an abort address following the XBEGIN
    // instruction.
    bool incomplete_instruction_fetch = (ef.vector == 14 and
                                         (ef.error_code & 0b10101 /* user space instruction fetch */) == 0b10100 and
                                         get_cr2() == get_user_page() + page_size and
                                         ef.ip == guest_ip);

    if (not incomplete_instruction_fetch)
      break;
  }

  return { (uint8_t)i, (uint8_t)ef.vector };
}

static void self_test_instruction_length()
{
  static const struct {
    size_t length;
    instruction_bytes instr;
  } tests[] {
    { 1, { 0x90 } },            // nop
    { 1, { 0xCC } },            // int3
    { 2, { 0xCD, 0x01 } },      // int 0x01
    { 2, { 0x00, 0x00 } },      // add [rax], al
    { 2, { 0xEB, 00 } },        // jmp 0x2
    { 5, { 0xE9, 0x00, 0x00, 0x00, 0x00 } }, // jmp 0x5
#ifdef __x86_64__
    { 6, { 0x66, 0xE9, 0x00, 0x00, 0x00, 0x00 } }, // jmp 0x6
    { 5, { 0x66, 0x48, 0x0f, 0x6e, 0xc0 } }, // movq xmm0, rax
    // lock add qword cs:[eax+4*eax+07e06df23h], 0efcdab89h
    { 15, { 0x2e, 0x67, 0xf0, 0x48,
            0x81, 0x84, 0x80, 0x23,
            0xdf, 0x06, 0x7e, 0x89,
            0xab, 0xcd, 0xef }},
#else
    { 4, { 0x66, 0xE9, 0x00, 0x00 } }, // jmp 0x4
    { 2, { 0x66, 0x48 } }, // dec ax
#endif
    { 6, { 0xC7, 0xF8, 0x00, 0x00, 0x00, 0x00 } }, // xbegin 0x6
    // This tests whether we fixup the direction flag when coming back from userspace.
    { 1, { 0xFD } }, // std
    // This tests whether we fixup segment registers when coming back from userspace.
    { 2, { 0x8e, 0xd8 } }, // mov ds, eax
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
  auto const disasm = disassemble(instr);

  // Prefix instruction, so it's easy to grep output.
  format("E ", hex(attempt.exception, 2, false), " O ");

  format(hex(disasm.instruction, 4, false), " ");

  if (disasm.instruction != X86_INS_INVALID and disasm.length != attempt.length) {
    // The CPU and Capstone successfully decoded an instruction, but they
    // disagree about the length. This is likely a Capstone bug.
    format("BUG ");
  } else if (attempt.exception != 6 and attempt.length <= array_size(instr.raw) and
             disasm.instruction == X86_INS_INVALID) {
    // The CPU successfully executed an instruction (no #UD exception), but
    // capstone is clueless what that is.
    //
    // Ignore cases where we ran over the instruction length limit.
    format("UNKN");
  } else {
    format("OK  ");
  }

  format(attempt.length >= array_size(instr.raw) ? "!" : " ");

  format(" |");
  for (size_t i = 0; i < attempt.length and i < array_size(instr.raw); i++) {
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

struct options {
  size_t prefixes = 0;
  const char *mode = nullptr;
};

// This will modify cmdline.
static options parse_and_destroy_cmdline(char *cmdline)
{
  options res;
  char *tok_state = nullptr;;

  for (char *tok = strtok_r(cmdline, " ", &tok_state); tok;
       tok = strtok_r(nullptr, " ", &tok_state)) {
    char *kv_state = nullptr;
    const char *key = strtok_r(tok, "=", &kv_state);
    const char *value = key ? strtok_r(nullptr, "", &kv_state) : nullptr;

    if (not value) continue;

    if (strcmp(key, "mode") == 0)
      res.mode = value;
    if (strcmp(key, "prefixes") == 0)
      res.prefixes = atoi(value);
  }

  return res;
}

void start(char *cmdline)
{
  print_logo();

  const auto options = parse_and_destroy_cmdline(cmdline);
  const auto sig = get_cpu_signature();
  format(">>> CPU is ", sig.vendor, " ", hex(sig.signature, 8, false), ".\n");

  setup_arch(options.mode);
#ifdef __x86_64__
  setup_disassembler(64);
#else
  setup_disassembler(32);
#endif

  format(">>> Executing self test.\n");
  self_test_instruction_length();
  disassemble_self_test();

  format(">>> Probing instruction space with up to ", options.prefixes,
         " legacy prefix", options.prefixes == 1 ? "" : "es",
         ".\n");

  // TODO Add option to control the number of prefix bytes as command line
  // parameter.
  search_engine search { options.prefixes };
  execution_attempt last_attempt;

  do {
    auto const &candidate = search.get_candidate();
    auto attempt = find_instruction_length(candidate);

    search.clear_after(attempt.length);

    if (is_interesting_change(last_attempt, attempt)) {
      search.start_over(attempt.length);

      if (attempt.length <= sizeof(candidate.raw))
        print_instruction(candidate, attempt);
    }

    last_attempt = attempt;
  } while (search.find_next_candidate());

  format(">>> Done!\n");

  // Reset
  outbi<0x64>(0xFE);
}
