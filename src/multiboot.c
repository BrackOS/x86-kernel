#include "multiboot.h"
#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint32_t type;
    uint32_t size;
} mb_tag_t;

static uint32_t mbi_addr = 0;

void multiboot_init(uint32_t addr) {
    mbi_addr = addr;
}

const void *multiboot_find_tag(uint32_t wanted) {
    if (mbi_addr == 0) return NULL;

    uint32_t total = *(const uint32_t *)(uintptr_t)mbi_addr;
    const uint8_t *p = (const uint8_t *)(uintptr_t)(mbi_addr + 8);  /* skip total_size + reserved */
    const uint8_t *end = (const uint8_t *)(uintptr_t)(mbi_addr + total);

    while (p < end) {
        const mb_tag_t *t = (const mb_tag_t *)p;
        if (t->type == MB_TAG_END) break;
        if (t->type == wanted) return p;
        p += (t->size + 7) & ~7u;   /* tags are 8-byte aligned */
    }
    return NULL;
}
