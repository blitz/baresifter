#pragma once

#include <cstdint>

struct cpuid_result {
  uint32_t eax;
  uint32_t ebx;
  uint32_t edx;
  uint32_t ecx;
};

struct cpu_signature {
  uint32_t signature;
  char vendor[3*4 + 1];
};

cpuid_result get_cpuid(uint32_t leaf, uint32_t subleaf = 0);
cpu_signature get_cpu_signature();
bool running_virtualized();

// Return the maximum supported standard CPUID level.
uint32_t get_cpuid_max_std_level();

// Return the maximum supported extended CPUID level.
uint32_t get_cpuid_max_ext_level();

// Returns true, if the CPU reports being able to use the NX bit.
bool has_nx();

// Returns true, if the CPU reports being able to use SMEP.
bool has_smep();
