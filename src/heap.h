#ifndef HEAP_H
#define HEAP_H

#include <stddef.h>
#include <stdint.h>

/* Simple free-list allocator for the kernel heap.
 * Tracks allocations in a linked list of free blocks.
 * Not thread-safe; suitable for single-threaded kernel use. */

void heap_init(void *start, size_t size);
void *kmalloc(size_t size);
void kfree(void *ptr);

/* For debugging */
void heap_stats(void);

#endif
