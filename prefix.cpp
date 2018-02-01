#include "prefix.hpp"

static int opcode_to_prefix_group(uint8_t byte)
{
  int group = -1;

  switch (byte) {
  case 0xF0:                    // LOCK
  case 0xF2:                    // REPNE
  case 0xF3:                    // REP
    group = 0;
    break;
  case 0x2E:                    // CS
  case 0x36:                    // SS
  case 0x3e:                    // DS
  case 0x26:                    // ES
  case 0x64:                    // FS
  case 0x65:                    // GS
    group = 1;
    break;
  case 0x66:                    // operand size override
    group = 2;
    break;
  case 0x67:                    // address size override
    group = 3;
    break;
  case 0x40 ... 0x4F:           // REX prefixes
    group = 4;
    break;
  }

  return group;
}

bool has_duplicated_prefixes(instruction_bytes const &instr)
{
  // Count prefix occurrence per group.
  int prefix_count[5] {};

  for (uint8_t b : instr.raw) {
    int group = opcode_to_prefix_group(b);
    if (group < 0)
      break;

    if (++prefix_count[group] > 1)
      return true;
  }

  return false;
}
