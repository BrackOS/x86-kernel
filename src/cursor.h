#ifndef CURSOR_H
#define CURSOR_H

#include <stdint.h>

#define CURSOR_WIDTH 8
#define CURSOR_HEIGHT 8

void cursor_init(void);
void cursor_draw(int x, int y);
void cursor_hide(void);
void cursor_update(int x, int y);
void cursor_set_color(uint32_t color);

#endif
