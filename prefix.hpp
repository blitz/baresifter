#pragma once

#include "disassemble.hpp"

bool has_duplicated_prefixes(instruction_bytes const &);
bool has_ordered_prefixes(instruction_bytes const &);
