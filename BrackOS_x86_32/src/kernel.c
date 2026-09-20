#include "vga.h"
#include "keyboard.h"
#include <stddef.h>

#define CMD_BUFFER_SIZE 256

static size_t my_strlen(const char *s) {
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

static int my_strncmp(const char *a, const char *b, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (a[i] != b[i]) return (unsigned char)a[i] - (unsigned char)b[i];
        if (a[i] == '\0') return 0;
    }
    return 0;
}

static void print_prompt(void) {
    vga_set_color(2, 0);
    vga_print("BrackOS> ");
    vga_set_color(7, 0);
}

static size_t read_line(char *buf, size_t max_len) {
    size_t len = 0;
    for (;;) {
        char c = keyboard_getchar();

        if (c == '\n') {
            vga_putchar('\n');
            buf[len] = '\0';
            return len;
        } else if (c == '\b') {
            if (len > 0) {
                len--;
                vga_backspace();
            }
        } else if (len < max_len - 1) {
            buf[len++] = c;
            vga_putchar(c);
        }
    }
}

static void run_command(char *line) {
    while (*line == ' ') line++;

    if (my_strlen(line) == 0) {
        return;
    }

    if (my_strncmp(line, "echo", 4) == 0 && (line[4] == ' ' || line[4] == '\0')) {
        const char *arg = line + 4;
        while (*arg == ' ') arg++;
        
        if (*arg == '\0') {
            vga_putchar('\n');
        } else {
            vga_print(arg);
            vga_putchar('\n');
        }
    }
    else if (my_strncmp(line, "clear", 5) == 0 && (line[5] == ' ' || line[5] == '\0')) {
        vga_clear();
    }
    else if (my_strncmp(line, "help", 4) == 0 && (line[4] == ' ' || line[4] == '\0')) {
        vga_print("Available commands:\n");
        vga_print("  echo <text>   - print text to screen\n");
        vga_print("  clear         - clear the screen\n");
        vga_print("  help          - show this help message\n");
    }
    else {
        vga_set_color(12, 0);
        vga_print("error: unknown command: ");
        vga_print(line);
        vga_putchar('\n');
        vga_set_color(7, 0);
    }
}

void kernel_main(void) {
    vga_init();
    vga_set_color(15, 0);
    vga_print("BrackOS x86-32 booted successfully.\n");
    vga_set_color(7, 0);
    vga_print("Type 'help' for a list of commands.\n\n");

    char cmd_buffer[CMD_BUFFER_SIZE];

    for (;;) {
        print_prompt();
        read_line(cmd_buffer, CMD_BUFFER_SIZE);
        run_command(cmd_buffer);
    }
}
