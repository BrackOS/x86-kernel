#ifndef MOUSE_H
#define MOUSE_H

#include <stdint.h>

typedef struct {
    int x;           // Absolute X position (0 to width)
    int y;           // Absolute Y position (0 to height)
    int dx;          // Relative X movement
    int dy;          // Relative Y movement
    uint8_t buttons; // Bitmask: bit 0=left, 1=right, 2=middle
    int left_pressed;
    int right_pressed;
    int middle_pressed;
} mouse_state_t;

// Universal mouse input interface (abstraction for PS/2, USB, etc)
typedef struct {
    void (*init)(void);
    void (*update)(void);
    mouse_state_t* (*get_state)(void);
    const char* (*get_name)(void);
} mouse_driver_t;

extern mouse_state_t mouse_state;
extern mouse_driver_t *active_mouse_driver;

// PS/2 specific driver
extern mouse_driver_t ps2_mouse_driver;

// Universal mouse functions
void mouse_init(void);
void mouse_update(void);
mouse_state_t* mouse_get(void);
const char* mouse_get_driver_name(void);

#endif
