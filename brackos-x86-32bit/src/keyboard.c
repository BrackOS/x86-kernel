#include "keyboard_tables.h"
#include "keyboard.h"
#include "io.h"
#include <stdint.h>
#include <stddef.h>

#define KBD_STATUS_PORT 0x64
#define KBD_DATA_PORT   0x60

static int shift_pressed = 0;
static int extended_key = 0;   /* set after an 0xE0 prefix byte */
static const char backspace_char = 0x08;  /* Ctrl-H, backspace

static inline void cpu_pause(void) {
    asm volatile("pause");
}





#define SC_LSHIFT 0x2A
#define SC_RSHIFT 0x36
#define SC_LSHIFT_REL (SC_LSHIFT | 0x80)
#define SC_RSHIFT_REL (SC_RSHIFT | 0x80)

/* Discard anything already waiting in the controller */
void keyboard_init(void) {
    shift_pressed = 0;
    extended_key = 0;
    while (inb(KBD_STATUS_PORT) & 1) {
        (void)inb(KBD_DATA_PORT);
    }
}

/* Non-blocking check: is a scancode waiting to be read? */
int keyboard_has_input(void) {
    return (inb(KBD_STATUS_PORT) & 1) != 0;
}

/* PS/2 keyboard scancode to ASCII mappings */




char keyboard_getchar(void) {
    for (;;) {
        if (!(inb(KBD_STATUS_PORT) & 1)) {
            cpu_pause();
            continue;
        }
        uint8_t sc = inb(KBD_DATA_PORT);

        /* Extended keys (arrows etc.) send 0xE0 and then the key code */
        if (sc == 0xE0) {
            extended_key = 1;
            continue;
        }
        if (extended_key) {
            extended_key = 0;
            if (sc & 0x80) continue;          /* key release */
            switch (sc) {
                case 0x48: return KEY_UP;
                case 0x50: return KEY_DOWN;
                case 0x4B: return KEY_LEFT;
                case 0x4D: return KEY_RIGHT;
                default:   continue;
            }
        }

        if (sc == SC_LSHIFT || sc == SC_RSHIFT) {
            shift_pressed = 1;
            continue;
        }
        if (sc == SC_LSHIFT_REL || sc == SC_RSHIFT_REL) {
            shift_pressed = 0;
            continue;
        }

        if (sc & 0x80) {
            continue;
        }

        if (sc >= 128) continue;

        char c = shift_pressed ? scancode_ascii_shift[sc] : scancode_ascii[sc];
        if (c != 0) {
            return c;
        }
    }
}
