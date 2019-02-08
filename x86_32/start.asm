  ; -*- Mode: nasm -*-

%define PAGE_SIZE 4096
%define MAX_CMDLINE_SIZE 256

bits 32

global _start, kern_stack
extern start, wait_forever, execute_constructors

section .bss
  kern_stack resb 4 * PAGE_SIZE
kern_stack_end:

  cmdline resb MAX_CMDLINE_SIZE

section .text._start
_mbheader:
align 4
  dd 1BADB002h                  ; magic
  dd 3h                         ; features
  dd -(3h + 1BADB002h)          ; checksum

align 8

_start:
  ; Is there a command line? If not, we keep the empty string as command line.
  test dword [ebx], 0x04
  jz no_cmdline

  ; Get command line pointer from multiboot info
  mov esi, [ebx + 16]

  ; Find end of string
  xor eax, eax
  mov edi, esi
  mov ecx, MAX_CMDLINE_SIZE - 1
  repne scasb

  ; Calculate string length
  sub edi, esi
  mov ecx, edi

  ; Copy string
  lea edi, [cmdline]
  rep movsb

no_cmdline:

  lea esp, [kern_stack_end]
  call execute_constructors
  lea eax, [cmdline]
  push wait_forever
  jmp start
