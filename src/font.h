#ifndef FONT_H
#define FONT_H

#include <stdint.h>

#define FONT_WIDTH 8
#define FONT_HEIGHT 8
#define FONT_CHAR_COUNT 128

extern const uint8_t font_data[FONT_CHAR_COUNT][FONT_HEIGHT];

void font_init(void);
void font_draw_char(int x, int y, char c, uint32_t fg, uint32_t bg);
void font_draw_string(int x, int y, const char *str, uint32_t fg, uint32_t bg);

#endif
