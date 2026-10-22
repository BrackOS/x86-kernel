#ifndef KEYBOARD_H
#define KEYBOARD_H

/* PS/2 keyboard scan codes */
#define SC_LSHIFT      0x2A
#define SC_RSHIFT      0x36
#define SC_LSHIFT_REL  0xAA
#define SC_RSHIFT_REL  0xB6

/* Special keys returned by keyboard_getchar() (control-character range) */
#define KEY_UP     0x11
#define KEY_DOWN   0x12
#define KEY_LEFT   0x13
#define KEY_RIGHT  0x14

void keyboard_init(void);
int keyboard_has_input(void);
char keyboard_getchar(void);

#endif
