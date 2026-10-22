#include "cursor.h"
#include "fb.h"

// 8×8 arrow cursor bitmap (top-left origin)
// Each row is a byte: bit 7=left, bit 0=right
const uint8_t cursor_bitmap[CURSOR_HEIGHT] = {
    0x80, // ████░░░░
    0xC0, // ██████░░
    0xE0, // ███████░
    0xF0, // ████████
    0xF8, // █████████
    0xFC, // ██████████
    0xFE, // ███████████
    0x00, // ░░░░░░░░
};

static int cursor_x = 0;
static int cursor_y = 0;
static uint32_t cursor_color = 0x00000000;  // Black
static uint32_t cursor_bg_color = 0xFFFFFFFF;  // White (background to restore)

void cursor_init(void) {
    cursor_x = 400;
    cursor_y = 300;
    cursor_color = fb_make_color(0, 0, 0);  // Black
}

void cursor_draw(int x, int y) {
    // Save current position
    cursor_x = x;
    cursor_y = y;
    
    // Draw arrow cursor
    for (int dy = 0; dy < CURSOR_HEIGHT; dy++) {
        uint8_t row = cursor_bitmap[dy];
        for (int dx = 0; dx < CURSOR_WIDTH; dx++) {
            // Check if pixel should be drawn (bit set = draw)
            if (row & (1 << (7 - dx))) {
                fb_set_pixel(x + dx, y + dy, cursor_color);
            }
        }
    }
}

void cursor_hide(void) {
    // Redraw background (for now, just use white)
    // In future, this would restore saved background pixels
    for (int dy = 0; dy < CURSOR_HEIGHT; dy++) {
        for (int dx = 0; dx < CURSOR_WIDTH; dx++) {
            fb_set_pixel(cursor_x + dx, cursor_y + dy, cursor_bg_color);
        }
    }
}

void cursor_update(int x, int y) {
    // Hide old cursor
    cursor_hide();
    
    // Draw new cursor
    cursor_draw(x, y);
}

void cursor_set_color(uint32_t color) {
    cursor_color = color;
}
