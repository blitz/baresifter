#pragma once

#include <cstddef>
#include <cstdint>

struct exception_frame {
  uint64_t r15;
  uint64_t r14;
  uint64_t r13;
  uint64_t r12;
  uint64_t r11;
  uint64_t r10;
  uint64_t r9;
  uint64_t r8;

  uint64_t rdi;
  uint64_t rsi;
  uint64_t rbp;
  uint64_t rbx;
  uint64_t rdx;
  uint64_t rcx;
  uint64_t rax;

  uint64_t vector;
  uint64_t error_code;
  uint64_t rip;
  uint64_t cs;
  uint64_t rflags;
  uint64_t rsp;
  uint64_t ss;
};

const size_t page_size = 4096;

// The user space page as a read-only user-accessible mapping.
uintptr_t get_user_page();

// The user space page as a read-write supervisor-accessible mapping.
char *get_user_page_backing();

// The entry point that is called by the assembly bootstrap code.
extern "C" void start();

// Try to execute a single userspace instruction and return the exception that
// resulted.
exception_frame execute_user(uintptr_t rip);

// Setup paging, descriptor tables and instruction set extensions.
void setup_arch();
