#include <cstdint>
#include <cstring>
#include <cstdlib>

#include "arch.hpp"
#include "cpuid.hpp"
#include "execution_attempt.hpp"
#include "logo.hpp"
#include "search.hpp"
#include "util.hpp"
#include "x86.hpp"
#include "cpu_features.hpp"

static execution_attempt find_instruction_length(cpu_features const &features,
						 instruction_bytes const &instr)
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
    // fetch page fault from userspace.
    //
    // The check for RIP is necessary, because otherwise we cannot correctly
    // guess the length of XBEGIN with an abort address following the XBEGIN
    // instruction.

    mword_t const pt_exc_instr = features.has_nx ? EXC_PF_ERR_I : 0;
    mword_t const pt_exc_mask = EXC_PF_ERR_P | EXC_PF_ERR_U | pt_exc_instr;
    mword_t const pt_exc_expect = EXC_PF_ERR_U | pt_exc_instr;

    bool const incomplete_instruction_fetch =
      ef.vector == 14 and
      (ef.error_code & pt_exc_mask) == pt_exc_expect and
      get_cr2() == get_user_page() + page_size and
      ef.ip == guest_ip;

    if (not incomplete_instruction_fetch)
      break;
  }

  return { (uint8_t)i, (uint8_t)ef.vector };
}

static void self_test_instruction_length(cpu_features const &features)
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

    // Intel and AMD disagree on this one, so don't do this as part of the self-test.
    // { 6, { 0x66, 0xE9, 0x00, 0x00, 0x00, 0x00 } }, // jmp 0x6

    // Qemu's decoder gets this wrong, so also don't do this in the self-test.
    // { 5, { 0x66, 0x48, 0x0f, 0x6e, 0xc0 } }, // movq xmm0, rax

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
    auto attempt = find_instruction_length(features, test.instr);
    if (attempt.length != test.length) {
      success = false;
    }
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
  format("EXC ", hex(attempt.exception, 2, false), " ");

  format(attempt.length >= array_size(instr.raw) ? "??" : "OK");

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
  // We allow this many prefixes. Limiting prefixes is useful, because
  // they make the search space explode.
  size_t prefixes = 0;

  // After how many instructions do we stop. Zero means don't stop.
  size_t stop_after = 0;

  // What prefixes to use. Zero means no prefixes are valid to use. The bits of the number are the opcode groups.
  size_t used_prefixes = 0xFF;

  // What prefixes to detect. Zero means no prefixes are valid. The bits of the number are the opcode groups.
  size_t detect_prefixes = 0xFF;
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

    if (strcmp(key, "prefixes") == 0)
      res.prefixes = atoi(value);
    if (strcmp(key, "stop_after") == 0)
      res.stop_after = atoi(value);
  }

  return res;
}

void start(cpu_features const &features, char *cmdline)
{
  print_logo();

  auto options = parse_and_destroy_cmdline(cmdline);
  const auto sig = get_cpu_signature();
  format(">>> CPU is ", sig.vendor, " ", hex(sig.signature, 8, false), ".\n");

  format(">>> Executing self test.\n");
  self_test_instruction_length(features);

  format(">>> Probing instruction space with up to ", options.prefixes,
         " legacy prefix", options.prefixes == 1 ? "" : "es",
         ".\n");
  if (options.stop_after)
    format(">>> Stopping after ", options.stop_after, " execution attemps.\n");

  search_engine search { options.prefixes, options.used_prefixes, options.detect_prefixes };
  execution_attempt last_attempt;

  do {
    auto const &candidate = search.get_candidate();
    auto attempt = find_instruction_length(features, candidate);

    search.clear_after(attempt.length);

    if (is_interesting_change(last_attempt, attempt)) {
      search.start_over(attempt.length);

      if (attempt.length <= sizeof(candidate.raw))
        print_instruction(candidate, attempt);
    }

    last_attempt = attempt;
  } while (--options.stop_after > 0 && search.find_next_candidate());

  format(">>> Done!\n");

  // Reset
  outbi<0x64>(0xFE);
}
