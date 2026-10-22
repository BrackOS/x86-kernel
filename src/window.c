#include "window.h"
#include "fb.h"
#include "font.h"

window_manager_t wm = {0};

void window_init(void) {
    wm.count = 0;
    for (int i = 0; i < MAX_WINDOWS; i++) {
        wm.windows[i].visible = 0;
    }
}

int window_create(int x, int y, int w, int h, const char *title, uint32_t bg_color) {
    if (wm.count >= MAX_WINDOWS) {
        return -1;
    }
    
    int id = wm.count;
    window_t *win = &wm.windows[id];
    
    win->x = x;
    win->y = y;
    win->width = w;
    win->height = h;
    win->bg_color = bg_color;
    win->z_order = id;
    win->visible = 1;
    
    int i = 0;
    while (i < 63 && title[i]) {
        win->title[i] = title[i];
        i++;
    }
    win->title[i] = '\0';
    
    wm.count++;
    return id;
}

void window_destroy(int id) {
    if (id < 0 || id >= wm.count) return;
    wm.windows[id].visible = 0;
}

void window_draw_all(void) {
    // Draw windows back to front (sorted by z_order)
    for (int z = 0; z < wm.count; z++) {
        for (int i = 0; i < wm.count; i++) {
            if (!wm.windows[i].visible) continue;
            if (wm.windows[i].z_order != z) continue;
            
            window_t *win = &wm.windows[i];
            
            // Fill window background
            fb_fill_rect(win->x, win->y, win->width, win->height, win->bg_color);
            
            // Draw window border (1px outline)
            uint32_t border_color = fb_make_color(64, 64, 64);
            
            // Top and bottom
            for (int x = win->x; x < win->x + win->width; x++) {
                fb_set_pixel(x, win->y, border_color);
                fb_set_pixel(x, win->y + win->height - 1, border_color);
            }
            
            // Left and right
            for (int y = win->y; y < win->y + win->height; y++) {
                fb_set_pixel(win->x, y, border_color);
                fb_set_pixel(win->x + win->width - 1, y, border_color);
            }
        }
    }
}

void window_clear_all(void) {
    wm.count = 0;
    for (int i = 0; i < MAX_WINDOWS; i++) {
        wm.windows[i].visible = 0;
    }
}

/* Fill in defaults for a window that was declared without them */
void window_init_buffer(window_t *win) {
    if (win->width <= 0)  win->width = 200;
    if (win->height <= 0) win->height = 100;
    if (win->bg_color == 0) win->bg_color = fb_make_color(192, 192, 192);
    if (win->x < 0) win->x = 0;
    if (win->y < 0) win->y = 0;
}

/* Draw one window: background, outline, and a title bar with its title */
void window_draw(const window_t *win) {
    if (!win->visible) return;

    const int bar_h = 16;
    uint32_t title_bg = fb_make_color(0, 0, 128);
    uint32_t title_fg = fb_make_color(255, 255, 255);
    uint32_t border   = fb_make_color(64, 64, 64);

    fb_fill_rect(win->x, win->y, win->width, win->height, win->bg_color);
    fb_fill_rect(win->x, win->y, win->width, bar_h, title_bg);
    font_draw_string(win->x + 4, win->y + 4, win->title, title_fg, title_bg);

    for (int x = win->x; x < win->x + win->width; x++) {
        fb_set_pixel(x, win->y, border);
        fb_set_pixel(x, win->y + win->height - 1, border);
    }
    for (int y = win->y; y < win->y + win->height; y++) {
        fb_set_pixel(win->x, y, border);
        fb_set_pixel(win->x + win->width - 1, y, border);
    }
}
