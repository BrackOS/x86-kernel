#include "gui_commands.h"
#include "fb.h"
#include "font.h"
#include "window.h"
#include "keyboard.h"

void gui_show_desktop(void) {
    // Clear screen to Windows blue
    uint32_t win_blue = fb_make_color(120, 172, 212);  // #78ACD4
    fb_clear(win_blue);
    
    // Draw taskbar at bottom (white bar)
    int taskbar_height = 40;
    int taskbar_y = fb.height - taskbar_height;
    uint32_t white = fb_make_color(255, 255, 255);
    fb_fill_rect(0, taskbar_y, fb.width, taskbar_height, white);
    
    // Draw taskbar separator
    uint32_t gray = fb_make_color(200, 200, 200);
    for (int x = 0; x < (int)fb.width; x++) {
        fb_set_pixel(x, taskbar_y, gray);
    }
    
    // Draw "BrackOS" text in taskbar
    uint32_t black = fb_make_color(0, 0, 0);
    font_draw_string(10, taskbar_y + 12, "BrackOS v1.1", black, white);
}

void gui_show_info_window(void) {
    gui_show_desktop();
    
    // Create info window in center
    window_init();
    window_create(300, 150, 400, 300, "System Info", fb_make_color(240, 240, 240));
    window_draw_all();
    
    uint32_t black = fb_make_color(0, 0, 0);
    uint32_t light_gray = fb_make_color(240, 240, 240);
    
    int x = 320;
    int y = 180;
    font_draw_string(x, y, "BrackOS v1.1", black, light_gray);
    y += 12;
    font_draw_string(x, y, "x86-64 Kernel", black, light_gray);
    y += 12;
    font_draw_string(x, y, "64-bit Long Mode", black, light_gray);
    y += 12;
    font_draw_string(x, y, "GRUB 2 Multiboot2", black, light_gray);
    y += 12;
    font_draw_string(x, y, "VGA Display", black, light_gray);
    y += 12;
    font_draw_string(x, y, "PS/2 Keyboard", black, light_gray);
}

void gui_show_commands_window(void) {
    gui_show_desktop();
    
    window_init();
    window_create(200, 100, 600, 500, "Commands", fb_make_color(240, 240, 240));
    window_draw_all();
    
    uint32_t black = fb_make_color(0, 0, 0);
    uint32_t light_gray = fb_make_color(240, 240, 240);
    
    int x = 220;
    int y = 130;
    
    const char *commands[] = {
        "help      - Show all commands",
        "info      - System information",
        "cpu       - CPU details",
        "mem       - Memory layout",
        "echo      - Print text",
        "clear     - Clear screen",
        "repeat    - Repeat command",
        "credits   - Project info",
        "shutdown  - Halt system",
        "reboot    - Restart system",
    };
    
    for (int i = 0; i < 10; i++) {
        font_draw_string(x, y, commands[i], black, light_gray);
        y += 12;
    }
}

void gui_show_about_window(void) {
    gui_show_desktop();
    
    window_init();
    window_create(350, 200, 300, 250, "About", fb_make_color(240, 240, 240));
    window_draw_all();
    
    uint32_t black = fb_make_color(0, 0, 0);
    uint32_t light_gray = fb_make_color(240, 240, 240);
    
    int x = 370;
    int y = 230;
    
    font_draw_string(x, y, "BrackOS", black, light_gray);
    y += 16;
    font_draw_string(x, y, "v1.1", black, light_gray);
    y += 12;
    font_draw_string(x, y, "Minimal 64-bit OS", black, light_gray);
    y += 12;
    font_draw_string(x, y, "x86-64 Architecture", black, light_gray);
    y += 12;
    font_draw_string(x, y, "Educational", black, light_gray);
    y += 12;
    font_draw_string(x, y, "Open Source", black, light_gray);
}

void gui_interactive_mode(void) {
    gui_show_desktop();
    
    uint32_t white = fb_make_color(255, 255, 255);
    
    // Display instructions
    font_draw_string(10, 20, "Press [I] for Info  [C] for Commands  [A] for About  [Q] to Quit", 
                    white, fb_make_color(120, 172, 212));
    
    // Simple key handler
    for (;;) {
        char c = keyboard_getchar();
        if (c == 'i' || c == 'I') {
            gui_show_info_window();
        } else if (c == 'c' || c == 'C') {
            gui_show_commands_window();
        } else if (c == 'a' || c == 'A') {
            gui_show_about_window();
        } else if (c == 'q' || c == 'Q') {
            break;
        }
    }
}
