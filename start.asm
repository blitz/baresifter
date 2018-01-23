  ; -*- Mode: nasm -*-

  bits 32

  extern main
  global _start

SECTION .text._start EXEC NOWRITE ALIGN=4
_mbheader:
align 4
  dd 1BADB002h            ; magic
  dd 3h                   ; features (page aligned modules, mem info, video mode table)
  dd -(3h + 1BADB002h)    ; checksum

_start:
  nop
  ret

  bits 64
  default rel
_start_64:
  nop
  ret
