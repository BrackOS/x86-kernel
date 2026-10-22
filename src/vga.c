#include "vga.h"

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;
static uint16_t* const VGA_MEMORY = (uint16_t*) 0xB8000;

static size_t row;
static size_t col;
static uint8_t color;
static uint16_t* buffer;

static inline uint8_t vga_entry_color(uint8_t fg, uint8_t bg) {
    return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char c, uint8_t color) {
    return (uint16_t) c | (uint16_t) color << 8;
}

void vga_set_color(uint8_t fg, uint8_t bg) {
    color = vga_entry_color(fg, bg);
}

void vga_clear(void) {
    buffer = VGA_MEMORY;
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            buffer[index] = vga_entry(' ', color);
        }
    }
    row = 0;
    col = 0;
}

void vga_init(void) {
    color = vga_entry_color(7, 0);
    vga_clear();
}

static void vga_scroll(void) {
    for (size_t y = 1; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            buffer[(y - 1) * VGA_WIDTH + x] = buffer[y * VGA_WIDTH + x];
        }
    }
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = vga_entry(' ', color);
    }
    row = VGA_HEIGHT - 1;
}

static void vga_newline(void) {
    col = 0;
    if (++row == VGA_HEIGHT) {
        vga_scroll();
    }
}

void vga_putchar(char c) {
    if (c == '\n') {
        vga_newline();
        return;
    }
    if (c == '\r') {
        col = 0;
        return;
    }

    const size_t index = row * VGA_WIDTH + col;
    buffer[index] = vga_entry((unsigned char) c, color);
    if (++col == VGA_WIDTH) {
        vga_newline();
    }
}

void vga_backspace(void) {
    if (col == 0 && row == 0) return;
    if (col == 0) {
        row--;
        col = VGA_WIDTH - 1;
    } else {
        col--;
    }
    buffer[row * VGA_WIDTH + col] = vga_entry(' ', color);
}

void vga_print(const char *str) {
    for (size_t i = 0; str[i] != '\0'; i++) {
        vga_putchar(str[i]);
    }
}
