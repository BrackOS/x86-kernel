#include "keyboard.h"
#include "io.h"
#include <stdint.h>

#define KBD_STATUS_PORT 0x64
#define KBD_DATA_PORT   0x60

static int shift_pressed = 0;

static inline void cpu_pause(void) {
    asm volatile("pause");
}

static const char scancode_ascii[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,
    'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,
    '\\','z','x','c','v','b','n','m',',','.','/',
    0,
    '*',
    0,
    ' ',
    0,
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

static const char scancode_ascii_shift[128] = {
    0,  27, '!','@','#','$','%','^','&','*','(',')','_','+','\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
    0,
    'A','S','D','F','G','H','J','K','L',':','"','~',
    0,
    '|','Z','X','C','V','B','N','M','<','>','?',
    0,
    '*',
    0,
    ' ',
    0,
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

#define SC_LSHIFT 0x2A
#define SC_RSHIFT 0x36
#define SC_LSHIFT_REL (SC_LSHIFT | 0x80)
#define SC_RSHIFT_REL (SC_RSHIFT | 0x80)

char keyboard_getchar(void) {
    for (;;) {
        if (!(inb(KBD_STATUS_PORT) & 1)) {
            cpu_pause();
            continue;
        }
        uint8_t sc = inb(KBD_DATA_PORT);

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
