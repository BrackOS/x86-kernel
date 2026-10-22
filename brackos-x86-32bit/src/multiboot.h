#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include <stdint.h>

/* Multiboot2 tag types used by the kernel */
#define MB_TAG_END          0
#define MB_TAG_MMAP         6
#define MB_TAG_FRAMEBUFFER  8

/* Store the Multiboot2 info pointer that GRUB passed to the kernel */
void multiboot_init(uint32_t mbi_addr);

/* Return the first tag of the given type, or NULL if GRUB did not pass it */
const void *multiboot_find_tag(uint32_t type);

#endif
