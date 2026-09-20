; boot.s -- Multiboot header + entry point
; This is what GRUB looks for to know how to load and start our kernel.

MBALIGN     equ  1<<0                  ; align loaded modules on page boundaries
MEMINFO     equ  1<<1                  ; provide memory map
FLAGS       equ  MBALIGN | MEMINFO     ; multiboot 'flag' field
MAGIC       equ  0x1BADB002            ; magic number GRUB looks for
CHECKSUM    equ -(MAGIC + FLAGS)       ; checksum required by spec

section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

section .bss
align 16
stack_bottom:
    resb 16384 ; 16 KiB stack
stack_top:

section .text
global _start
extern kernel_main
_start:
    mov esp, stack_top     ; set up the stack
    cli                    ; interrupts stay off until we explicitly enable them
    call kernel_main        ; hand off to C

.hang:
    hlt
    jmp .hang
