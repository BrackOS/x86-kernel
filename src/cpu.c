#include "cpu.h"
#include "vga.h"
#include <stdint.h>

static void cpuid(uint32_t leaf, uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d) {
    asm volatile("cpuid" : "=a"(*a), "=b"(*b), "=c"(*c), "=d"(*d) : "a"(leaf), "c"(0));
}

void cpu_print_info(void) {
    uint32_t a, b, c, d;
    char vendor[13];

    cpuid(0, &a, &b, &c, &d);
    *(uint32_t *)&vendor[0] = b;
    *(uint32_t *)&vendor[4] = d;
    *(uint32_t *)&vendor[8] = c;
    vendor[12] = 0;

    vga_print("CPU Information:\n");
    vga_print("  Vendor: ");
    vga_print(vendor);
    vga_print("\n");

    /* Brand string: leaves 0x80000002..0x80000004, 16 bytes each */
    cpuid(0x80000000, &a, &b, &c, &d);
    if (a >= 0x80000004) {
        char brand[49];
        uint32_t *w = (uint32_t *)brand;
        for (uint32_t leaf = 0x80000002; leaf <= 0x80000004; leaf++) {
            cpuid(leaf, &a, &b, &c, &d);
            int base = (int)(leaf - 0x80000002) * 4;
            w[base + 0] = a;
            w[base + 1] = b;
            w[base + 2] = c;
            w[base + 3] = d;
        }
        brand[48] = 0;
        const char *p = brand;
        while (*p == ' ') p++;   /* brand strings are often left-padded */
        vga_print("  Model: ");
        vga_print(p);
        vga_print("\n");
    }

    /* Feature flags */
    cpuid(1, &a, &b, &c, &d);
    vga_print("  Features: ");
    vga_print((d & (1u << 9))  ? "APIC " : "");
    vga_print((d & (1u << 25)) ? "SSE " : "");
    vga_print((d & (1u << 26)) ? "SSE2 " : "");
    vga_print("\n");

    cpuid(0x80000000, &a, &b, &c, &d);
    if (a >= 0x80000001) {
        cpuid(0x80000001, &a, &b, &c, &d);
        vga_print("  Long mode: ");
        vga_print((d & (1u << 29)) ? "supported\n" : "not supported\n");
    }
}
