#ifndef VGA_H
#define VGA_H

#include <stdint.h>
#include <stddef.h>

void vga_init(void);
void vga_clear(void);
void vga_putchar(char c);
void vga_print(const char *str);
void vga_set_color(uint8_t fg, uint8_t bg);
void vga_backspace(void);

#endif
