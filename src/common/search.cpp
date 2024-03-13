#include <cstddef>
#include <cstring>

#include "search.hpp"
#include "util.hpp"

static constexpr int opcode_to_prefix_group(uint8_t byte, size_t detect_prefixes_)
{
  int group = -1;

  switch (byte) {
  case 0xF0:                    // LOCK
  case 0xF2:                    // REPNE
  case 0xF3:                    // REP
    if (detect_prefixes_ & (1<<0)) //To detect?
    {
      group = 0;
    }
    break;
  case 0x2E:                    // CS
  case 0x36:                    // SS
  case 0x3e:                    // DS
  case 0x26:                    // ES
  case 0x64:                    // FS
  case 0x65:                    // GS
    if (detect_prefixes_ & (1<<1)) //To detect?
    {
      group = 1;
    }
    break;
  case 0x66:                    // operand size override
    if (detect_prefixes_ & (1<<2)) //To detect?
    {
      group = 2;
    }
    break;
  case 0x67:                    // address size override
    if (detect_prefixes_ & (1<<3)) //To detect?
    {
      group = 3;
    }
    break;
  case 0x40 ... 0x4F:           // REX prefixes
    if (detect_prefixes_ & (1<<4)) //To detect?
    {
      group = 4;
    }
    break;
  }

  return group;
}

prefix_group_lut::prefix_group_lut(size_t detect_prefixes_)
{
  for (size_t i = 0; i < array_size(data); i++) {
    data[i] = (int8_t)opcode_to_prefix_group((uint8_t)i,detect_prefixes_);
  }
}

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

  bool has_unused_prefixes(size_t used_prefixes_) const
  {
    //Detect used_prefixes and filter them out.
      size_t b = 1;
      for (size_t i = 0; i < array_size(count); i++, b <<= 1)
      {
          if (count[i] and ((used_prefixes_ & b) == 0)) //Prefix not to be used?
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

static prefix_state analyze_prefixes(prefix_group_lut const &group_lut_, instruction_bytes const &instr)
{
  prefix_state state;

  for (size_t i = 0; i < sizeof(instr.raw); i++) {
    int group = group_lut_.data[instr.raw[i]];
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

  auto const state = analyze_prefixes(group_lut_, current_);

  // Duplicated prefixes make the search space explode without generating
  // insight. Also enforce order on prefixes to further reduce search space.
  // And also filter out prefixes that are declared not to be used.
  if (state.total_prefix_bytes() > max_prefixes_ or
      state.has_duplicated_prefixes() or
      state.has_unused_prefixes(used_prefixes_) or
      not state.has_ordered_prefixes()) {
    goto again;
  }

  return true;
}
