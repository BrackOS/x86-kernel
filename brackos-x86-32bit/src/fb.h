#ifndef FB_H
#define FB_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint32_t *buffer;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t bpp;
} framebuffer_t;

extern framebuffer_t fb;

void fb_init(void);
void fb_set_pixel(int x, int y, uint32_t color);
uint32_t fb_make_color(uint8_t r, uint8_t g, uint8_t b);
void fb_clear(uint32_t color);
void fb_fill_rect(int x, int y, int w, int h, uint32_t color);
void fb_draw_line(int x0, int y0, int x1, int y1, uint32_t color);

#endif
