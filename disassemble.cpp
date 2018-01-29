#include "disassemble.hpp"
#include "util.hpp"

#include <capstone.h>
#include <string.h>

static char heap[16 << 10];
static size_t heap_pos = 0;

static void *malloc(size_t size)
{
  format("malloc: ", hex((uintptr_t)__builtin_return_address(0)), " size ", size, "\n");

  if (heap_pos + size > sizeof(heap)) {
    format("Out of memory\n");
    wait_forever();
  }

  size_t old_pos = heap_pos;
  heap_pos += (size + 0x7) & ~0x7;
  return &heap[old_pos];
}

static void *calloc(size_t nmemb, size_t size)
{
  format("calloc: ", hex((uintptr_t)__builtin_return_address(0)),
         " nmemb ", nmemb, " size ", size, "\n");

  return malloc(nmemb * size);
}

static void *realloc(void *, size_t)
{
  format("realloc: ", hex((uintptr_t)__builtin_return_address(0)), "\n");
  return nullptr;
}

static int vsnprintf(char *d, size_t n, const char *s, va_list)
{
  // This is only called when Capstone is not built with CAPSTONE_DIET.

  format("vsnprintf: ", hex((uintptr_t)__builtin_return_address(0)), "\n");
  strncpy(d, s, n);

  // FIXME Very poor! This only works in the happy case.
  return strlen(s);;
}

static void free(void *)
{
  format("free: ", hex((uintptr_t)__builtin_return_address(0)), "\n");
}

#define cs_check(expr) do { \
    int __err__ = expr; \
    if (__err__ != CS_ERR_OK) { format("Capstone error ", __err__, "\n"); goto fail; } \
  } while (0)

void disassemble_self_test()
{
  csh handle;

  cs_opt_mem opt_mem {};
  static const uint8_t bytes[] = "\x48\x8b\x05\xb8\x13\x00\x00";
  size_t count = sizeof(bytes) - 1;
  cs_insn *insn = nullptr;
  uint8_t const *code = bytes;
  uint64_t address = 0x1000;


  opt_mem.malloc = malloc;
  opt_mem.calloc = calloc;
  opt_mem.realloc = realloc;
  opt_mem.free = free;
  opt_mem.vsnprintf = vsnprintf;

  cs_check(cs_option(0, CS_OPT_MEM, (uintptr_t)&opt_mem));
  cs_check(cs_open(CS_ARCH_X86, CS_MODE_64, &handle));

  insn = cs_malloc(handle);

  format("cs_disasm_iter\n");
  count = cs_disasm_iter(handle, &code, &count, &address, insn);
  format("count ", count, " id ", insn->id, " insn ", insn->mnemonic, " op ", insn->op_str, "\n");

  format("Capstone: OK\n");
  return;

 fail:
  format("Capstone: b0rken!\n");
  wait_forever();
}
