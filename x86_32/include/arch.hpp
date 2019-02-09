#pragma once

#include <cstddef>
#include <cstdint>

struct exception_frame {
  uint32_t edi;
  uint32_t esi;
  uint32_t ebp;
  uint32_t ign;                 // esp from pusha
  uint32_t ebx;
  uint32_t edx;
  uint32_t ecx;
  uint32_t eax;

  uint32_t vector;
  uint32_t error_code;
  uint32_t ip;
  uint32_t cs;
  uint32_t eflags;              // These might not exist when the kernel faults
  uint32_t esp;
  uint32_t ss;
};

const size_t page_size = 4096;

// The user space page as a read-only user-accessible mapping.
uintptr_t get_user_page();

// The user space page as a read-write supervisor-accessible mapping.
char *get_user_page_backing();

// The entry point that is called by the assembly bootstrap code.
extern "C" void start(char *cmdline);

// Try to execute a single userspace instruction and return the exception that
// resulted.
exception_frame execute_user(uintptr_t rip);

// Setup paging, descriptor tables and instruction set extensions.
void setup_arch(const char *mode);
