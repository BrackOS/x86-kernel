#include "idt.h"
#include <stdint.h>
#include "io.h"
#include "vga.h"
#include "timer.h"

#define IDT_ENTRIES 256

static idt_entry_t idt[IDT_ENTRIES] __attribute__((aligned(16)));
static idt_ptr_t idt_ptr;

// PIC (Programmable Interrupt Controller) ports
#define PIC1_CMD   0x20
#define PIC1_DATA  0x21
#define PIC2_CMD   0xA0
#define PIC2_DATA  0xA1

#define PIC_EOI    0x20

// Interrupt/Exception handlers
void exception_handler(int exception_num) {
    vga_set_color(12, 0);  // Red text
    vga_print("EXCEPTION: ");
    vga_print_int(exception_num);
    vga_putchar('\n');
    vga_set_color(7, 0);
    vga_print("System halted.\n");
    for (;;) {
        asm volatile("cli; hlt");
    }
    
    // Halt
    for (;;) {
        asm volatile("hlt");
    }
}

void irq_handler(int vector) {
    /* The PIC is remapped so hardware IRQ n arrives as vector 32 + n */
    int irq = vector - 32;

    switch (irq) {
        case IRQ_TIMER:
            timer_irq_handler();
            break;
        case IRQ_KEYBOARD:
        case IRQ_MOUSE:
        case IRQ_ATA_DISK1:
        case IRQ_ATA_DISK2:
            /* Devices are currently polled; nothing to do yet */
            break;
        default:
            break;
    }

    /* End of interrupt: the slave PIC needs one too for IRQs 8-15 */
    if (irq >= 8) {
        outb(PIC2_CMD, PIC_EOI);
    }
    outb(PIC1_CMD, PIC_EOI);
}

// Set an IDT entry (layout depends on CPU mode; see idt.h)
void idt_set_entry(int index, uintptr_t handler, uint16_t selector, uint8_t type) {
#if defined(__x86_64__)
    idt[index].offset_low = handler & 0xFFFF;
    idt[index].offset_mid = (handler >> 16) & 0xFFFF;
    idt[index].offset_high = (handler >> 32) & 0xFFFFFFFF;
    idt[index].selector = selector;
    idt[index].ist = 0;
    idt[index].type_attr = type | 0x80;  // Present bit (0x80)
    idt[index].reserved = 0;
#else
    idt[index].offset_low = handler & 0xFFFF;
    idt[index].offset_high = (handler >> 16) & 0xFFFF;
    idt[index].selector = selector;
    idt[index].zero = 0;
    idt[index].type_attr = type | 0x80;  // Present bit (0x80)
#endif
}

// Initialize PIC (Programmable Interrupt Controller)
static void pic_init(void) {
    // ICW1: Start initialization
    outb(PIC1_CMD, 0x11);
    outb(PIC2_CMD, 0x11);
    
    // ICW2: Set interrupt offsets
    outb(PIC1_DATA, 0x20);  // Master PIC: interrupts 32-39
    outb(PIC2_DATA, 0x28);  // Slave PIC: interrupts 40-47
    
    // ICW3: Set cascading
    outb(PIC1_DATA, 0x04);  // Master: slave at IRQ2
    outb(PIC2_DATA, 0x02);  // Slave: cascaded to master
    
    // ICW4: Set 8086 mode
    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);
    
    // Mask all interrupts for now
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}

// Enable specific IRQ
void pic_enable_irq(int irq) {
    uint16_t port = (irq < 8) ? PIC1_DATA : PIC2_DATA;
    uint8_t mask = inb(port);
    mask &= ~(1 << (irq % 8));  // Clear bit to enable
    outb(port, mask);
}

// Disable specific IRQ
void pic_disable_irq(int irq) {
    uint16_t port = (irq < 8) ? PIC1_DATA : PIC2_DATA;
    uint8_t mask = inb(port);
    mask |= (1 << (irq % 8));   // Set bit to disable
    outb(port, mask);
}

// Initialize IDT and PIC
void idt_init(void) {
    // Clear IDT
    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt_set_entry(i, 0, 0, 0);
    }
    
    // Set up exception handlers (0-31 are exceptions)
    // For now, set them to dummy handlers
    // In a real OS, each would have its own handler
    
    // Set up IRQ handlers (32-47)
    // IRQ 0: Timer
    idt_set_entry(32, (uintptr_t)irq_stub_0, 0x08, IDT_GATE_INTERRUPT);
    
    // IRQ 1: Keyboard
    idt_set_entry(33, (uintptr_t)irq_stub_1, 0x08, IDT_GATE_INTERRUPT);
    
    // IRQ 12: Mouse
    idt_set_entry(44, (uintptr_t)irq_stub_12, 0x08, IDT_GATE_INTERRUPT);
    
    // IRQ 14: ATA Primary
    idt_set_entry(46, (uintptr_t)irq_stub_14, 0x08, IDT_GATE_INTERRUPT);
    
    // Initialize PIC
    pic_init();
    
    // Load IDT
    idt_ptr.limit = (sizeof(idt) - 1);
    idt_ptr.base = (uintptr_t)&idt[0];
    
    asm volatile("lidt %0" : : "m"(idt_ptr));
    
    // Enable interrupts
    asm volatile("sti");
    
    vga_print("IDT: Initialized\n");
}

// Disable interrupts
void interrupts_disable(void) {
    asm volatile("cli");
}

// Enable interrupts
void interrupts_enable(void) {
    asm volatile("sti");
}
