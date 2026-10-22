; idt_stubs32.s -- 32-bit protected-mode interrupt/exception entry stubs
;
; 32-bit code uses the cdecl calling convention (arguments on the stack,
; not in registers), so this is not just a word-size shrink of idt_stubs.s:
; the argument has to be pushed for the call rather than loaded into rdi.

bits 32

extern exception_handler
extern irq_handler

%macro ISR_NOERRCODE 1
    global isr_stub_%1
    isr_stub_%1:
        push 0              ; dummy error code
        push %1             ; exception number
        jmp isr_common
%endmacro

%macro IRQ_STUB 2
    global irq_stub_%1
    irq_stub_%1:
        push 0
        push %2             ; vector number (32 + IRQ)
        jmp irq_common
%endmacro

section .text

isr_common:
    pusha                       ; save eax,ecx,edx,ebx,esp(orig),ebp,esi,edi
    mov eax, [esp + 32]         ; vector number, pushed before pusha's 32 bytes
    push eax
    call exception_handler
    add esp, 4
    popa
    add esp, 8                  ; discard vector + dummy error code
    iretd

irq_common:
    pusha
    mov eax, [esp + 32]
    push eax
    call irq_handler
    add esp, 4
    popa
    add esp, 8
    iretd

ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 8

IRQ_STUB 0, 32
IRQ_STUB 1, 33
IRQ_STUB 12, 44
IRQ_STUB 14, 46
