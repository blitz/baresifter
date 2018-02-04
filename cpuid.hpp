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

cpuid_result get_cpuid(uint32_t leaf);
cpu_signature get_cpu_signature();
bool running_virtualized();
