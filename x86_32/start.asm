  ; -*- Mode: nasm -*-

bits 32

global _start

section .text._start
_mbheader:
align 4
  dd 1BADB002h                  ; magic
  dd 3h                         ; features
  dd -(3h + 1BADB002h)          ; checksum

align 8

_start:
  ; XXX
  ud2
