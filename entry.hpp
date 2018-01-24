#pragma once

#include <cstddef>

// Total number of interrupt handlers
constexpr size_t irq_entry_count = 32;

// An array of interrupt entry functions
extern "C" char irq_entry_start[];
extern "C" char irq_entry_end[];
