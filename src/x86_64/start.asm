  ; -*- Mode: nasm -*-

%define PAGE_SIZE 4096
%define IA32_EFER 0xC0000080
%define IA32_EFER_LME 0x100
%define IA32_EFER_NXE 0x800

%define PTE_P (1 << 0)
%define PTE_W (1 << 1)
%define PTE_U (1 << 2)
%define PTE_PS (1 << 7)

%define CR4_PAE (1 << 5)
%define CR4_OSFXSR (1 << 9)
%define CR4_OSXSAVE (1 << 18)
%define CR4_SMEP (1 << 20)      ; Available since Ivy Bridge
%define CR0_PE (1 << 0)
%define CR0_MP (1 << 1)
%define CR0_WP (1 << 16)
%define CR0_PG (1 << 31)

%define XCR0_X87 (1 << 0)
%define XCR0_SSE (1 << 1)
%define XCR0_AVX (1 << 2)

%define MAX_CMDLINE_SIZE 256

bits 32

extern start, _image_start, wait_forever, execute_constructors, setup_arch
extern boot_pml4, boot_pdpt, boot_pd
global _start, kern_stack

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

  ; The GDT has some unused fields to make the selectors match the GDT we load
  ; later.
_boot_gdt:
  dw _boot_gdt_end - _boot_gdt - 1
  dd _boot_gdt
  dw 0
  dq 0
  dq 0x00a09b0000000000         ; Code
  dq 0
  dq 0x00a0930000000000         ; Data
_boot_gdt_end:

%define RING0_CODE_SELECTOR 0x10
%define RING0_DATA_SELECTOR 0x20

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

  ; Fill out PML4
  mov eax, boot_pdpt
  or eax, PTE_P | PTE_W | PTE_U
  mov dword [boot_pml4], eax

  ; Fill out PDPT
  mov eax, boot_pd
  or eax, PTE_P | PTE_W
  mov dword [boot_pdpt], eax

  ; Fill out PDE
  mov eax, _image_start
  mov ebx, eax
  shr ebx, 18
  or eax, PTE_P | PTE_W | PTE_PS
  mov dword [boot_pd + ebx], eax

  ; Load page table
  mov eax, boot_pml4
  mov cr3, eax

  ; Long mode initialization. See Intel SDM Vol. 3 Chapter 9.8.5.
  ; Also enable supervisor-mode execution prevention and non-integer instruction
  ; sets (SSE) that are required for generic x86-64 code.
  mov eax, CR4_PAE | CR4_SMEP | CR4_OSFXSR | CR4_OSXSAVE
  mov cr4, eax

  ; See avx.cpp for AVX initialization.
  mov eax, XCR0_X87 | XCR0_SSE
  xor edx, edx
  xor ecx, ecx
  xsetbv

  xor edx, edx
  mov eax, IA32_EFER_LME | IA32_EFER_NXE
  mov ecx, IA32_EFER
  wrmsr

  ; Enable paging
  mov eax, CR0_PE | CR0_MP | CR0_WP | CR0_PG
  mov cr0, eax

  lgdt [_boot_gdt]
  jmp RING0_CODE_SELECTOR:_start_long

bits 64
default rel
_start_long:

  mov eax, RING0_DATA_SELECTOR
  mov ss, eax
  mov ds, eax
  mov es, eax
  mov fs, eax
  mov gs, eax

  lea rsp, [kern_stack_end]
  call execute_constructors

  call setup_arch

  mov rdi, rax
  lea rsi, [cmdline]

  call start
  call wait_forever
