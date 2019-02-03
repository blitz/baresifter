  ; -*- Mode: nasm -*-

%define PAGE_SIZE 4096

bits 32

global _start, kern_stack
extern start, wait_forever, execute_constructors

section .bss
  kern_stack resb 4 * PAGE_SIZE
kern_stack_end:

section .text._start
_mbheader:
align 4
  dd 1BADB002h                  ; magic
  dd 3h                         ; features
  dd -(3h + 1BADB002h)          ; checksum

align 8

_start:
  lea esp, [kern_stack_end]
  call execute_constructors
  push wait_forever
  jmp start
