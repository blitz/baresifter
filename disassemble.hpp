#pragma once

#include <capstone/x86.h>

void setup_disassembler();
void disassemble_self_test();

// A raw set of bytes representing an instruction (potentially).
struct instruction_bytes {
  // x86 instructions are at most 15 bytes long.
  uint8_t raw[15];

  template <typename... T>
  constexpr instruction_bytes(T... v)
  : raw {(uint8_t)v...}
  {}
};

// The result of capstone.
struct decoded_instruction {
  enum x86_insn instruction = X86_INS_INVALID;
  uint8_t length = 0;
};

inline bool operator==(decoded_instruction const &a, decoded_instruction const &b)
{
  return (a.instruction == b.instruction) and (a.length == b.length);
}

inline bool operator!=(decoded_instruction const &a, decoded_instruction const &b)
{
  return not (a == b);
}

decoded_instruction disassemble(instruction_bytes const &);
