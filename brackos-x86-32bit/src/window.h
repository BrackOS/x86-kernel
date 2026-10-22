#ifndef WINDOW_H
#define WINDOW_H

#include <stdint.h>

#define MAX_WINDOWS 8

typedef struct {
    int x, y;
    int width, height;
    uint32_t bg_color;
    int z_order;
    int visible;
    char title[64];
} window_t;

typedef struct {
    window_t windows[MAX_WINDOWS];
    int count;
} window_manager_t;

extern window_manager_t wm;

void window_init(void);
void window_init_buffer(window_t *win);
void window_draw(const window_t *win);
int window_create(int x, int y, int w, int h, const char *title, uint32_t bg_color);
void window_destroy(int id);
void window_draw_all(void);
void window_clear_all(void);

#endif
