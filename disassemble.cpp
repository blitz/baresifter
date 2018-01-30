#include "disassemble.hpp"
#include "util.hpp"

#include <capstone.h>
#include <string.h>

static char heap[16 << 10];
static size_t heap_pos = 0;

// We have a primitive bump-pointer allocation. This is fine, because Capstone
// will only allocate memory at the beginning and never release it.
static void *malloc(size_t size)
{
  if (heap_pos + size > sizeof(heap)) {
    format("Out of memory! Need ", (heap_pos + size) - sizeof(heap), " bytes.\n");
    wait_forever();
  }

  size_t old_pos = heap_pos;

  // Align by 8.
  heap_pos += (size + 0x7) & ~0x7;
  return &heap[old_pos];
}

static void *calloc(size_t nmemb, size_t size)
{
  return malloc(nmemb * size);
}

#define NOT_IMPLEMENTED(n) do { \
    format(n ": not implemented. Called by ", hex((uintptr_t)__builtin_return_address(0)), "\n"); \
  } while (0)

static void *realloc(void *, size_t)
{
  NOT_IMPLEMENTED("realloc");
  return nullptr;
}

static int vsnprintf(char *d, size_t n, const char *s, va_list)
{
  // This is only called when Capstone is not built with CAPSTONE_DIET.

  NOT_IMPLEMENTED("vsnprintf");

  // FIXME Very poor! This only works in the happy case.
  strncpy(d, s, n);
  return strlen(s);;
}

static void free(void *)
{
  NOT_IMPLEMENTED("free");
}

#define cs_check(expr) do { \
    int __err__ = expr; \
    if (__err__ != CS_ERR_OK) { format("Capstone error ", __err__, "\n"); goto fail; } \
  } while (0)

static csh handle;
static cs_insn *insn;

void setup_disassembler()
{
  cs_opt_mem opt_mem {};

  opt_mem.malloc = malloc;
  opt_mem.calloc = calloc;
  opt_mem.realloc = realloc;
  opt_mem.free = free;
  opt_mem.vsnprintf = vsnprintf;

  cs_check(cs_option(0, CS_OPT_MEM, (uintptr_t)&opt_mem));
  cs_check(cs_open(CS_ARCH_X86, CS_MODE_64, &handle));

  insn = cs_malloc(handle);

  return;

 fail:
  format("Capstone did not initialize!\n");
  wait_forever();
}

decoded_instruction disassemble(instruction_bytes const &instr)
{
  size_t count = sizeof(instr.raw);
  uint8_t const *code = instr.raw;
  uint64_t address = 0;

  if (cs_disasm_iter(handle, &code, &count, &address, insn)) {
    uintptr_t length = code - instr.raw;
    return { (enum x86_insn)insn->id, (uint8_t)length };
  } else {
    return { /* invalid */ };
  }
}

void disassemble_self_test()
{
  static const struct {
    instruction_bytes bytes;
    decoded_instruction expected;
  } tests[] {
    { { 0x90 }, { X86_INS_NOP, 1 } },
    { { 0xe9, 0x00, 0x00, 0x00, 0x00 }, { X86_INS_JMP, 5 }},
    // XXX Known capstone issue https://github.com/aquynh/capstone/issues/776
    //{ { 0x66, 0xe9, 0x00, 0x00, 0x00, 0x00 }, { X86_INS_JMP, 6 }},
  };
  bool success = true;

  for (auto const &test : tests) {
    auto res = disassemble(test.bytes);

    if (res != test.expected)
      success = false;
  }

  format("Capstone: ", (success ? "OK" : "b0rken!"), "\n");
  if (not success) {
    wait_forever();
  }
}
