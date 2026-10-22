#include "heap.h"
#include "vga.h"
#include <stdint.h>

#define HEAP_MIN_BLOCK 32   /* Minimum block size to track */

typedef struct heap_block {
    struct heap_block *next;
    size_t size;            /* Size of this block (including header) */
    int free;               /* 1 if free, 0 if allocated */
} heap_block_t;

static heap_block_t *heap_head = NULL;
static uint8_t *heap_start = NULL;
static size_t heap_total = 0;

void heap_init(void *start, size_t size) {
    heap_start = (uint8_t *)start;
    heap_total = size;
    heap_head = (heap_block_t *)start;
    heap_head->next = NULL;
    heap_head->size = size;
    heap_head->free = 1;
}

void *kmalloc(size_t size) {
    if (size == 0) return NULL;

    /* Account for the block header */
    size_t needed = size + sizeof(heap_block_t);

    /* Find first free block large enough */
    heap_block_t *block = heap_head;
    while (block) {
        if (block->free && block->size >= needed) {
            /* Split the block if there's leftover space */
            if (block->size > needed + HEAP_MIN_BLOCK) {
                heap_block_t *new_block = (heap_block_t *)((uint8_t *)block + needed);
                new_block->next = block->next;
                new_block->size = block->size - needed;
                new_block->free = 1;
                block->next = new_block;
            }
            block->size = needed;
            block->free = 0;
            return (void *)((uint8_t *)block + sizeof(heap_block_t));
        }
        block = block->next;
    }

    return NULL;   /* Out of memory */
}

void kfree(void *ptr) {
    if (ptr == NULL) return;

    /* Find the block header (just before the pointer) */
    heap_block_t *block = (heap_block_t *)((uint8_t *)ptr - sizeof(heap_block_t));
    if ((uint8_t *)block < heap_start) return;   /* Sanity check */

    block->free = 1;

    /* Coalesce with next block if it's also free */
    if (block->next && block->next->free) {
        block->size += block->next->size;
        block->next = block->next->next;
    }

    /* Coalesce with previous block if it's also free */
    if (block != heap_head) {
        heap_block_t *prev = heap_head;
        while (prev && prev->next != block) prev = prev->next;
        if (prev && prev->free) {
            prev->size += block->size;
            prev->next = block->next;
        }
    }
}

void heap_stats(void) {
    vga_print("Heap stats:\n");
    int free_blocks = 0, used_blocks = 0;
    size_t free_bytes = 0, used_bytes = 0;
    heap_block_t *block = heap_head;
    while (block) {
        if (block->free) {
            free_blocks++;
            free_bytes += block->size;
        } else {
            used_blocks++;
            used_bytes += block->size;
        }
        block = block->next;
    }
    vga_print("  Free blocks: ");
    vga_print_int(free_blocks);
    vga_print(" (");
    vga_print_int(free_bytes / 1024);
    vga_print(" KB)\n");
    vga_print("  Used blocks: ");
    vga_print_int(used_blocks);
    vga_print(" (");
    vga_print_int(used_bytes / 1024);
    vga_print(" KB)\n");
}
