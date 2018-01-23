  ; -*- Mode: nasm -*-

%define PAGE_SIZE 4096
%define IA32_EFER 0xC0000080
%define IA32_EFER_LME 0x100

%define PTE_P (1 << 0)
%define PTE_W (1 << 1)
%define PTE_U (1 << 2)
%define PTE_PS (1 << 7)

%define CR4_PAE (1 << 5)
%define CR0_PG (1 << 31)

  bits 32

  extern start, _image_start
  global _start

section .bss
align PAGE_SIZE
  boot_pml4 resq 512
  boot_pdpt resq 512
  boot_pd   resq 512

  kern_stack resb 4 * PAGE_SIZE
kern_stack_end:

section .text._start
_mbheader:
align 4
  dd 1BADB002h                  ; magic
  dd 3h                         ; features
  dd -(3h + 1BADB002h)          ; checksum

align 8
_boot_gdt:
  dq 0
  ; TODO Nice constants please
  dq 0x00a09b0000000000         ; Code
  dq 0x00a0930000000000         ; Data
_boot_gdt_end:

_boot_gdt_ptr:
  dw _boot_gdt_end - _boot_gdt - 1
  dd _boot_gdt

_start:

  ; Fill out PML4
  lea eax, [boot_pdpt]
  or eax, PTE_P | PTE_W | PTE_U
  mov dword [boot_pml4], eax

  ; Fill out PDPT
  lea eax, [boot_pd]
  or eax, PTE_P | PTE_W | PTE_U
  mov dword [boot_pdpt], eax

  ; Fill out PDE
  lea eax, [_image_start]
  mov ebx, eax
  shr ebx, 18
  or eax, PTE_P | PTE_W | PTE_PS
  mov dword [boot_pd + ebx], eax

  ; Load page table
  lea eax, [boot_pml4]
  mov cr3, eax

  ; Long mode initialization. See Intel SDM Vol. 3 Chapter 9.8.5.
  mov eax, cr4
  or eax, CR4_PAE
  mov cr4, eax

  xor edx, edx
  mov eax, IA32_EFER_LME
  mov ecx, IA32_EFER
  wrmsr

  ; Enable paging
  mov eax, cr0
  or eax, CR0_PG
  mov cr0, eax

  lgdt [_boot_gdt_ptr]
  jmp 8:_start_long

  bits 64
  default rel
_start_long:

  ; TODO set segment registers

  lea rsp, [kern_stack_end]
  call start
_dead:
  cli
  hlt
  jmp _dead
