#include "meminfo.h"
#include "vga.h"
#include "multiboot.h"
#include <stdint.h>
#include <stddef.h>

/* Memory map tag layout (Multiboot2 spec) */
typedef struct {
    uint32_t type;          /* 6 = memory map */
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
} mb_mmap_tag_t;

typedef struct {
    uint64_t base;
    uint64_t length;
    uint32_t type;          /* 1 = available RAM */
    uint32_t reserved;
} mb_mmap_entry_t;

#define MB_MEM_AVAILABLE  1

static const mb_mmap_tag_t *find_mmap_tag(void) {
    return (const mb_mmap_tag_t *)multiboot_find_tag(MB_TAG_MMAP);
}

#if defined(__x86_64__)
static void print_u64(uint64_t v) {
    char buf[21];
    int i = 20;
    buf[i] = 0;
    if (v == 0) { vga_print("0"); return; }
    while (v && i > 0) { buf[--i] = (char)('0' + (v % 10)); v /= 10; }
    vga_print(&buf[i]);
}
#else
/* i386 has no hardware 64-bit divide. These values (RAM sizes in KB/MB)
 * always fit in 32 bits here, so print the low 32 bits with a native
 * 32-bit divide instead of requiring libgcc's software 64-bit routines. */
static void print_u64(uint64_t v) {
    uint32_t n = (uint32_t)v;
    char buf[11];
    int i = 10;
    buf[i] = 0;
    if (n == 0) { vga_print("0"); return; }
    while (n && i > 0) { buf[--i] = (char)('0' + (n % 10)); n /= 10; }
    vga_print(&buf[i]);
}
#endif

static void print_hex64(uint64_t v) {
    static const char hex[] = "0123456789ABCDEF";
    char buf[17];
    for (int i = 15; i >= 0; i--) {
        buf[i] = hex[v & 0xF];
        v >>= 4;
    }
    buf[16] = 0;
    vga_print("0x");
    vga_print(buf);
}

void meminfo_print(void) {
    const mb_mmap_tag_t *tag = find_mmap_tag();

    vga_print("Memory Map (reported by GRUB):\n");
    if (!tag || tag->entry_size == 0) {
        vga_print("  Not available (kernel was not loaded by Multiboot2)\n");
        return;
    }

    uint32_t count = (tag->size - sizeof(mb_mmap_tag_t)) / tag->entry_size;
    const uint8_t *base = (const uint8_t *)tag + sizeof(mb_mmap_tag_t);
    uint64_t usable = 0;

    for (uint32_t i = 0; i < count; i++) {
        const mb_mmap_entry_t *e = (const mb_mmap_entry_t *)(base + i * tag->entry_size);
        if (e->type == MB_MEM_AVAILABLE) {
            usable += e->length;
            vga_print("  ");
            print_hex64(e->base);
            vga_print("  ");
            print_u64(e->length / 1024);
            vga_print(" KB  available\n");
        }
    }

    vga_print("  Usable RAM: ");
    print_u64(usable / (1024 * 1024));
    vga_print(" MB\n");
}
