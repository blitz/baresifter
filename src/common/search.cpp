#include <cstddef>
#include <cstring>

#include "search.hpp"
#include "util.hpp"

struct prefix_lut {
  int8_t data[256];
};

static constexpr int opcode_to_prefix_group(uint8_t byte)
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
#ifndef __x86_64__
  case 0x40 ... 0x4F:           // REX prefixes
    group = 4;
    break;
  }
#endif

  return group;
}

static constexpr prefix_lut create_prefix_group_lut()
{
  prefix_lut group_lut {};

  for (size_t i = 0; i < array_size(group_lut.data); i++) {
    group_lut.data[i] = (int8_t)opcode_to_prefix_group((uint8_t)i);
  }

  return group_lut;
}

static prefix_lut prefix_group_lut {create_prefix_group_lut()};

// Encapsulates which prefixes are there, where and how many there are.
struct prefix_state {
  uint8_t count[5] {};          // Count of prefixes in each group.
  uint8_t position[5] {};       // Position of each prefix (tracked per group)

  size_t total_prefix_bytes() const
  {
    size_t sum = 0;

    for (auto c : count) {
      sum += c;
    }

    return sum;
  }

  bool has_duplicated_prefixes() const
  {
    for (auto c : count) {
      if (c >= 2)
        return true;
    }

    return false;
  }

  // We assume no duplicated prefixes here.
  bool has_ordered_prefixes() const
  {
    // TODO There has to be a better way to express this. This also generates an
    // amazing number of conditional jumps with clang. Maybe use non-short-circuit
    // boolean operators.
    for (size_t i = 0; i < array_size(position) - 1; i++)
      for (size_t j = i + 1; j < array_size(position); j++)
        // Use non-short-circuit operators to reduce the number of conditional
        // jumps.
        if (count[i] and count[j] and (position[i] > position[j]))
          return false;

    return true;
  }
};

static prefix_state analyze_prefixes(instruction_bytes const &instr)
{
  prefix_state state;

  for (size_t i = 0; i < sizeof(instr.raw); i++) {
    int group = prefix_group_lut.data[instr.raw[i]];
    if (group < 0)
      break;

    state.count[group]++;
    state.position[group] = i;
  }

  return state;
}

void search_engine::clear_after(size_t pos)
{
  if (pos < sizeof(current_.raw))
    memset(current_.raw + pos, 0, sizeof(current_.raw) - pos);
}

void search_engine::start_over(size_t length)
{
  increment_at_ = length - 1;
}

bool search_engine::find_next_candidate()
{
 again:
  current_.raw[increment_at_]++;

  if (current_.raw[increment_at_] == 0) {
    // We've wrapped at our current position, so go left one byte. If we hit
    // the beginning, we are done.
    if (unlikely(increment_at_-- == 0)) {
      return false;
    }

    goto again;
  }

  auto const state = analyze_prefixes(current_);

  // Duplicated prefixes make the search space explode without generating
  // insight. Also enforce order on prefixes to further reduce search space.
  if (state.total_prefix_bytes() > max_prefixes_ or
      state.has_duplicated_prefixes() or
      not state.has_ordered_prefixes()) {
    goto again;
  }

  return true;
}
