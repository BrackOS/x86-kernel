; boot32.s -- Multiboot2 header + x86 (32-bit protected mode) entry point
;
; GRUB already hands control to the kernel in 32-bit protected mode with
; paging off, so unlike boot.s (the x86-64 entry), this file does not need
; to touch PAE, long mode, or page tables -- it only needs its own GDT and
; a stack before jumping into C. This is what lets BrackOS run on CPUs
; that do not support 64-bit long mode at all.

bits 32

extern __kernel_start
extern __kernel_load_end
extern __kernel_bss_end

MULTIBOOT2_MAGIC equ 0xe85250d6
MULTIBOOT2_ARCH equ 0  ; i386
MULTIBOOT2_HEADER_LEN equ (multiboot_header_end - multiboot_header_start)
MULTIBOOT2_CHECKSUM equ -(MULTIBOOT2_MAGIC + MULTIBOOT2_ARCH + MULTIBOOT2_HEADER_LEN)

section .multiboot
align 8
multiboot_header_start:
    dd MULTIBOOT2_MAGIC
    dd MULTIBOOT2_ARCH
    dd MULTIBOOT2_HEADER_LEN
    dd MULTIBOOT2_CHECKSUM

    align 8
    dw 2                        ; address tag
    dw 0
    dd 24
    dd __kernel_start
    dd __kernel_start
    dd __kernel_load_end
    dd __kernel_bss_end

    align 8
    dw 3                        ; entry address tag
    dw 0
    dd 12
    dd _start

    align 8
    dw 0                        ; end tag
    dw 0
    dd 8
multiboot_header_end:

section .bss
align 16
stack_bottom:
    resb 16384
stack_top:

section .text
global _start
extern kmain

_start:
    ; GRUB leaves the multiboot info pointer in ebx; keep it safe in esi
    ; (esi is callee-/our-own-choice here, not touched by the GDT setup)
    mov esi, ebx

    lgdt [gdt32_pointer]
    jmp 0x08:.reload_segments

.reload_segments:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov esp, stack_top

    push esi            ; mbi_addr argument for kmain(uint32_t) (cdecl: on the stack)
    call kmain

.hang:
    cli
    hlt
    jmp .hang

; Flat 32-bit GDT: null, code (0x08), data (0x10)
align 8
gdt32:
    dq 0
    ; Code: base=0, limit=0xFFFFF (x4K = 4GB), present, ring0, exec/read, 32-bit, 4K granularity
    dw 0xFFFF, 0x0000
    db 0x00, 10011010b, 11001111b, 0x00
    ; Data: base=0, limit=0xFFFFF, present, ring0, read/write, 32-bit, 4K granularity
    dw 0xFFFF, 0x0000
    db 0x00, 10010010b, 11001111b, 0x00
gdt32_end:

gdt32_pointer:
    dw gdt32_end - gdt32 - 1
    dd gdt32
