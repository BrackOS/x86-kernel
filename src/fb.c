#include "fb.h"
#include "multiboot.h"
#include <stdint.h>

framebuffer_t fb = {0};

/* Multiboot2 framebuffer tag (type 8) */
typedef struct {
    uint32_t type;
    uint32_t size;
    uint64_t addr;
    uint32_t pitch;
    uint32_t width;
    uint32_t height;
    uint8_t  bpp;
    uint8_t  fb_type;
    uint16_t reserved;
} __attribute__((packed)) mb_fb_tag_t;

void fb_init(void) {
    const mb_fb_tag_t *tag = (const mb_fb_tag_t *)multiboot_find_tag(MB_TAG_FRAMEBUFFER);
    if (tag && tag->bpp == 32 && tag->addr < 0x100000000ULL) {
        fb.buffer = (uint32_t *)(uintptr_t)tag->addr;
        fb.width  = tag->width;
        fb.height = tag->height;
        fb.pitch  = tag->pitch;
        fb.bpp    = 32;
        return;
    }

    /* No framebuffer from GRUB: fall back to the usual QEMU VGA address */
    if (!fb.buffer) {
        fb.buffer = (uint32_t *)0xfd000000;
        fb.width = 1024;
        fb.height = 768;
        fb.pitch = 1024 * 4;
        fb.bpp = 32;
    }
}

void fb_set_pixel(int x, int y, uint32_t color) {
    if (x < 0 || x >= (int)fb.width || y < 0 || y >= (int)fb.height) {
        return;
    }
    fb.buffer[y * (fb.pitch / 4) + x] = color;
}

uint32_t fb_make_color(uint8_t r, uint8_t g, uint8_t b) {
    return (0xFF << 24) | (b << 16) | (g << 8) | r;
}

void fb_clear(uint32_t color) {
    for (int y = 0; y < (int)fb.height; y++) {
        for (int x = 0; x < (int)fb.width; x++) {
            fb_set_pixel(x, y, color);
        }
    }
}

void fb_fill_rect(int x, int y, int w, int h, uint32_t color) {
    for (int dy = 0; dy < h; dy++) {
        for (int dx = 0; dx < w; dx++) {
            fb_set_pixel(x + dx, y + dy, color);
        }
    }
}

void fb_draw_line(int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    for (;;) {
        fb_set_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}
