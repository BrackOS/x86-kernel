#include "paper.h"
#include "vga.h"
#include "keyboard.h"
#include "fat32.h"
#include "string.h"
#include <stdint.h>

#define KEY_ESC        27
#define TEXT_ROWS      23     /* rows used for text; status bar goes on row 24 */
#define MAX_TEXT_COLS  79     /* longest line we store (wider lines get clipped) */
#define TAB_WIDTH      4

typedef struct {
    char lines[PAPER_MAX_LINES][PAPER_MAX_LINE_LEN];
    int  line_count;
    int  cur_line;
    int  cur_col;
    int  view_top;
    int  dirty;
    uint32_t dir_cluster;
    char name[13];
    char message[40];
} paper_state_t;

static paper_state_t ed;
static uint8_t file_buf[PAPER_MAX_LINES * PAPER_MAX_LINE_LEN];

static int cur_len(void) {
    return (int)strlen(ed.lines[ed.cur_line]);
}

/* ---------- Loading and saving ---------- */

static void load_file(void) {
    ed.line_count = 1;
    ed.lines[0][0] = 0;

    int n = fat32_read_file(ed.dir_cluster, ed.name, file_buf, sizeof(file_buf));
    if (n < 0) {
        strcpy(ed.message, "New file");
        return;
    }

    int line = 0, col = 0;
    for (int i = 0; i < n; i++) {
        char c = (char)file_buf[i];
        if (c == '\r') continue;
        if (c == '\n') {
            ed.lines[line][col] = 0;
            if (line + 1 >= PAPER_MAX_LINES) break;
            line++;
            col = 0;
            continue;
        }
        if (col < MAX_TEXT_COLS) {
            ed.lines[line][col++] = c;
        }
    }
    ed.lines[line][col] = 0;
    ed.line_count = line + 1;
    strcpy(ed.message, "Loaded");
}

static void save_file(void) {
    int len = 0;
    for (int i = 0; i < ed.line_count; i++) {
        const char *s = ed.lines[i];
        while (*s) file_buf[len++] = (uint8_t)*s++;
        file_buf[len++] = '\n';
    }

    if (fat32_write_file(ed.dir_cluster, ed.name, file_buf, (uint32_t)len) == 0) {
        ed.dirty = 0;
        strcpy(ed.message, "Saved");
    } else {
        strcpy(ed.message, "SAVE FAILED");
    }
}

/* ---------- Editing operations ---------- */

static void insert_char(char c) {
    int len = cur_len();
    if (len >= MAX_TEXT_COLS) return;
    char *line = ed.lines[ed.cur_line];
    for (int i = len + 1; i > ed.cur_col; i--) {
        line[i] = line[i - 1];
    }
    line[ed.cur_col] = c;
    ed.cur_col++;
    ed.dirty = 1;
}

static void split_line(void) {
    if (ed.line_count >= PAPER_MAX_LINES) return;

    /* Make room for the new line below the current one */
    for (int i = ed.line_count; i > ed.cur_line + 1; i--) {
        strcpy(ed.lines[i], ed.lines[i - 1]);
    }
    /* Move the text after the cursor down into the new line */
    strcpy(ed.lines[ed.cur_line + 1], &ed.lines[ed.cur_line][ed.cur_col]);
    ed.lines[ed.cur_line][ed.cur_col] = 0;

    ed.line_count++;
    ed.cur_line++;
    ed.cur_col = 0;
    ed.dirty = 1;
}

static void delete_before_cursor(void) {
    if (ed.cur_col > 0) {
        char *line = ed.lines[ed.cur_line];
        int len = cur_len();
        for (int i = ed.cur_col - 1; i < len; i++) {
            line[i] = line[i + 1];
        }
        ed.cur_col--;
        ed.dirty = 1;
        return;
    }
    if (ed.cur_line == 0) return;

    /* At the start of a line: join it onto the line above */
    int prev_len = (int)strlen(ed.lines[ed.cur_line - 1]);
    int this_len = cur_len();
    if (prev_len + this_len > MAX_TEXT_COLS) return;

    strcpy(&ed.lines[ed.cur_line - 1][prev_len], ed.lines[ed.cur_line]);
    for (int i = ed.cur_line; i < ed.line_count - 1; i++) {
        strcpy(ed.lines[i], ed.lines[i + 1]);
    }
    ed.line_count--;
    ed.cur_line--;
    ed.cur_col = prev_len;
    ed.dirty = 1;
}

static void move_left(void) {
    if (ed.cur_col > 0) {
        ed.cur_col--;
    } else if (ed.cur_line > 0) {
        ed.cur_line--;
        ed.cur_col = cur_len();
    }
}

static void move_right(void) {
    if (ed.cur_col < cur_len()) {
        ed.cur_col++;
    } else if (ed.cur_line + 1 < ed.line_count) {
        ed.cur_line++;
        ed.cur_col = 0;
    }
}

static void move_vertical(int delta) {
    int target = ed.cur_line + delta;
    if (target < 0 || target >= ed.line_count) return;
    ed.cur_line = target;
    if (ed.cur_col > cur_len()) ed.cur_col = cur_len();
}

/* ---------- Display ---------- */

static void append_str(char *dst, int *n, const char *s) {
    while (*s && *n < 79) dst[(*n)++] = *s++;
}

static void append_int(char *dst, int *n, int v) {
    char tmp[12];
    int t = 0;
    if (v == 0) tmp[t++] = '0';
    while (v > 0 && t < 11) { tmp[t++] = (char)('0' + v % 10); v /= 10; }
    while (t > 0) dst[(*n)++] = tmp[--t];
}

static void draw(void) {
    /* Scroll so the cursor stays visible */
    if (ed.cur_line < ed.view_top) ed.view_top = ed.cur_line;
    if (ed.cur_line >= ed.view_top + TEXT_ROWS) ed.view_top = ed.cur_line - TEXT_ROWS + 1;

    vga_clear();

    for (int r = 0; r < TEXT_ROWS; r++) {
        int ln = ed.view_top + r;
        if (ln < ed.line_count) {
            char out[PAPER_MAX_LINE_LEN + 2];
            int n = 0;
            const char *s = ed.lines[ln];
            int len = (int)strlen(s);
            for (int c = 0; c <= len; c++) {
                if (ln == ed.cur_line && c == ed.cur_col) out[n++] = '|';
                if (c < len) out[n++] = s[c];
            }
            out[n] = 0;
            vga_print(out);
        }
        vga_print("\n");
    }

    /* Status bar (no trailing newline, so the screen does not scroll) */
    char status[81];
    int n = 0;
    append_str(status, &n, "PAPER ");
    append_str(status, &n, ed.name);
    append_str(status, &n, "  Ln ");
    append_int(status, &n, ed.cur_line + 1);
    append_str(status, &n, " Col ");
    append_int(status, &n, ed.cur_col + 1);
    if (ed.dirty) append_str(status, &n, " *");
    append_str(status, &n, "  ");
    append_str(status, &n, ed.message);
    append_str(status, &n, "  ESC=save+exit");
    status[n] = 0;
    vga_print(status);
}

/* ---------- Main loop ---------- */

void paper_open_file(const char *name, uint32_t dir_cluster) {
    memset(&ed, 0, sizeof(ed));
    ed.dir_cluster = dir_cluster;
    strcpy(ed.name, name);
    load_file();

    for (;;) {
        draw();
        char c = keyboard_getchar();

        switch (c) {
            case KEY_ESC:
                save_file();
                vga_clear();
                vga_print(ed.dirty ? "Could not save file.\n" : "File saved.\n");
                return;
            case '\n':  split_line(); break;
            case '\b':  delete_before_cursor(); break;
            case '\t':
                for (int i = 0; i < TAB_WIDTH; i++) insert_char(' ');
                break;
            case KEY_LEFT:  move_left(); break;
            case KEY_RIGHT: move_right(); break;
            case KEY_UP:    move_vertical(-1); break;
            case KEY_DOWN:  move_vertical(1); break;
            default:
                if (c >= 32 && c < 127) insert_char(c);
                break;
        }
        if (ed.message[0] && c != KEY_ESC) ed.message[0] = 0;
    }
}
