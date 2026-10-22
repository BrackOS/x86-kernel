#include "vga.h"
#include "keyboard.h"
#include "io.h"
#include "idt.h"
#include "fb.h"
#include "font.h"
#include "window.h"
#include "mouse.h"
#include "cursor.h"
#include "recovery.h"
#include "fat32.h"
#include "fs_shell.h"
#include "paper.h"
#include "rtc.h"
#include "timer.h"
#include "cpu.h"
#include "meminfo.h"
#include "heap.h"
#include "multiboot.h"
#include <stdint.h>
#include "string.h"

/* Command buffer */
#define CMD_BUFFER_SIZE 256
char cmd_buffer[CMD_BUFFER_SIZE];
int cmd_pos = 0;

/* GUI mode flag */
int gui_mode = 0;

void vga_print_int(int num) {
    if (num == 0) {
        vga_print("0");
        return;
    }
    
    char buffer[32];
    int i = 0;
    int negative = (num < 0);
    if (negative) num = -num;
    
    while (num > 0) {
        buffer[i++] = (num % 10) + '0';
        num /= 10;
    }
    
    if (negative) vga_print("-");
    while (i > 0) vga_print(&buffer[--i]);
}

/* Format time with leading zeros */
void print_time_component(uint8_t val) {
    if (val < 10) vga_print("0");
    vga_print_int(val);
}

void process_command(char *cmd) {
    if (strlen(cmd) == 0) return;
    
    if (strcmp(cmd, "help") == 0) {
        vga_print("BrackOS v1.7 Commands:\n");
        vga_print("  help       - This message\n");
        vga_print("  echo STR   - Print text\n");
        vga_print("  clear      - Clear screen\n");
        vga_print("  info       - System info\n");
        vga_print("  mem        - Memory layout\n");
        vga_print("  cpu        - CPU info\n");
        vga_print("  time       - Current time (HH:MM:SS)\n");
        vga_print("  date       - Current date (YYYY-MM-DD)\n");
        vga_print("  ver        - Version\n");
        vga_print("  uptime     - System uptime\n");
        vga_print("  credits    - Project credits\n");
        vga_print("  mouse      - Mouse info\n");
        vga_print("  fs         - Filesystem info\n");
        vga_print("  gui        - GUI mode\n");
        vga_print("  shutdown   - Halt\n");
        vga_print("  reboot     - Reboot\n");
        vga_print("  repeat N CMD - Run command N times\n");
        vga_print("\n");
        vga_print("Filesystem commands (A: drive):\n");
        vga_print("  ls [PATH]  - List directory\n");
        vga_print("  cd DIR     - Change directory\n");
        vga_print("  mkdir NAME - Create directory\n");
        vga_print("  nf NAME    - Create new file\n");
        vga_print("  paper FILE - Edit file with paper\n");
        vga_print("  rm FILE    - Delete file\n");
        vga_print("  rmdir DIR  - Delete directory\n");
    }
    else if (strncmp(cmd, "echo ", 5) == 0) {
        vga_print(&cmd[5]);
        vga_print("\n");
    }
    else if (strcmp(cmd, "clear") == 0) {
        vga_clear();
    }
    else if (strcmp(cmd, "info") == 0) {
        vga_print("BrackOS v1.7 Information\n");
        vga_print("========================\n");
        vga_print("OS: BrackOS Experimental Kernel\n");
        vga_print("Arch: x86-64 (Long Mode)\n");
        vga_print("Boot: Multiboot2\n");
        vga_print("VGA: 80x25 text mode\n");
        vga_print("GUI: 1024x768@32bpp VBE\n");
        vga_print("IDT: INITIALIZED\n");
        vga_print("Interrupts: ENABLED (STI)\n");
        vga_print("Timer: IRQ0 (PIT @100Hz)\n");
        vga_print("RTC: CMOS enabled\n");
        vga_print("Filesystem: FAT32 (A: drive)\n");
        vga_print("Editor: Paper text editor\n");
        vga_print("Recovery: Enabled\n");
    }
    else if (strcmp(cmd, "mem") == 0) {
        meminfo_print();
    }
    else if (strcmp(cmd, "cpu") == 0) {
        cpu_print_info();
    }
    else if (strcmp(cmd, "time") == 0) {
        rtc_time_t now = rtc_get_time();
        print_time_component(now.hour);
        vga_print(":");
        print_time_component(now.minute);
        vga_print(":");
        print_time_component(now.second);
        vga_print("\n");
    }
    else if (strcmp(cmd, "date") == 0) {
        rtc_time_t now = rtc_get_time();
        vga_print_int(now.year);
        vga_print("-");
        print_time_component(now.month);
        vga_print("-");
        print_time_component(now.day);
        vga_print("\n");
    }
    else if (strcmp(cmd, "uptime") == 0) {
        uptime_t up = rtc_get_uptime();
        vga_print("Uptime: ");
        vga_print_int(up.days);
        vga_print(" days, ");
        vga_print_int(up.hours);
        vga_print(":");
        print_time_component(up.minutes);
        vga_print(":");
        print_time_component(up.seconds);
        vga_print("\n");
    }
    else if (strncmp(cmd, "sleep ", 6) == 0) {
        int seconds = 0;
        for (size_t i = 6; i < strlen(cmd); i++) {
            if (cmd[i] >= '0' && cmd[i] <= '9') {
                seconds = seconds * 10 + (cmd[i] - '0');
            }
        }
        if (seconds > 0) {
            vga_print("Sleeping ");
            vga_print_int(seconds);
            vga_print(" seconds...\n");
            timer_sleep_ms(seconds * 1000);
            vga_print("Done.\n");
        } else {
            vga_print("Usage: sleep <seconds>\n");
        }
    }
    else if (strcmp(cmd, "ver") == 0) {
        vga_print("BrackOS v1.7 (x86-64)\n");
        vga_print("Timer + RTC + Full Clock Functions\n");
    }
    else if (strcmp(cmd, "credits") == 0) {
        vga_print("BrackOS Credits:\n");
        vga_print("  Creator: BrackOS Team\n");
        vga_print("  Bootloader: GRUB 2.04\n");
        vga_print("  Tools: GCC, Binutils, NASM\n");
    }
    else if (strcmp(cmd, "mouse") == 0) {
        vga_print("Mouse Driver Status:\n");
        mouse_state_t state = *mouse_get();
        vga_print("  Position: (");
        vga_print_int(state.x);
        vga_print(", ");
        vga_print_int(state.y);
        vga_print(")\n");
        vga_print("  Buttons: L=");
        vga_print_int(state.left_pressed);
        vga_print(" R=");
        vga_print_int(state.right_pressed);
        vga_print(" M=");
        vga_print_int(state.middle_pressed);
        vga_print("\n");
    }
    else if (strcmp(cmd, "fs") == 0) {
        vga_print("Filesystem Status:\n");
        if (fat32_fs.boot.bytes_per_sector > 0) {
            vga_print("  Type: FAT32\n");
            vga_print("  Sector size: ");
            vga_print_int(fat32_fs.boot.bytes_per_sector);
            vga_print(" bytes\n");
            vga_print("  Cluster size: ");
            vga_print_int(fat32_fs.boot.sectors_per_cluster);
            vga_print(" sectors\n");
            vga_print("  Root cluster: ");
            vga_print_int(fat32_fs.root_dir_cluster);
            vga_print("\n");
        } else {
            vga_print("  Not initialized\n");
        }
    }
    else if (strcmp(cmd, "gui") == 0) {
        gui_mode = 1;
    }
    else if (strcmp(cmd, "shutdown") == 0) {
        interrupts_disable();
        vga_print("Halting CPU...\n");
        __asm__ volatile("cli; hlt");
    }
    else if (strcmp(cmd, "reboot") == 0) {
        interrupts_disable();
        vga_print("Rebooting...\n");
        for (int i = 0; i < 100; i++) {
            outb(0x64, 0xFE);
        }
        __asm__ volatile("cli; hlt");
    }
    /* Filesystem commands */
    else if (strcmp(cmd, "ls") == 0) {
        fs_ls(0);
    }
    else if (strncmp(cmd, "ls ", 3) == 0) {
        fs_ls(&cmd[3]);
    }
    else if (strcmp(cmd, "cd /") == 0 || strcmp(cmd, "cd A:/") == 0) {
        fs_cd("/");
    }
    else if (strncmp(cmd, "cd ", 3) == 0) {
        fs_cd(&cmd[3]);
    }
    else if (strncmp(cmd, "mkdir ", 6) == 0) {
        fs_mkdir(&cmd[6]);
    }
    else if (strncmp(cmd, "nf ", 3) == 0) {
        fs_nf(&cmd[3]);
    }
    else if (strncmp(cmd, "rm ", 3) == 0) {
        fs_rm(&cmd[3]);
    }
    else if (strncmp(cmd, "rmdir ", 6) == 0) {
        fs_rmdir(&cmd[6]);
    }
    else if (strncmp(cmd, "paper ", 6) == 0) {
        paper_open_file(&cmd[6], current_dir.cluster);
    }
    else if (strncmp(cmd, "repeat ", 7) == 0) {
        int count = 0;
        int i = 7;
        while (cmd[i] >= '0' && cmd[i] <= '9') {
            count = count * 10 + (cmd[i] - '0');
            i++;
        }
        while (i < (int)strlen(cmd) && cmd[i] == ' ') i++;
        
        if (count > 0 && i < (int)strlen(cmd)) {
            char *subcmd = &cmd[i];
            for (int r = 0; r < count; r++) {
                process_command(subcmd);
            }
        }
    }
    else {
        vga_print("Unknown command: ");
        vga_print(cmd);
        vga_print("\n");
    }
}

void gui_mode_loop(void) {
    fb_init();
    font_init();
    window_init();
    mouse_init();
    
    fb_fill_rect(0, 0, 1024, 768, 0x78ACD4);
    fb_fill_rect(0, 728, 1024, 40, 0xFFFFFF);
    
    window_t info_win = {
        .x = 50, .y = 50, .width = 400, .height = 200,
        .title = "System Info", .visible = 1, .z_order = 0
    };
    window_init_buffer(&info_win);
    window_draw(&info_win);
    
    vga_print("GUI mode: Press Q to exit\n");
    
    while (1) {
        mouse_update();
        cursor_draw(mouse_get()->x, mouse_get()->y);
        
        if (keyboard_has_input()) {
            uint8_t scancode = inb(0x60);
            if (scancode == 0x10) {
                break;
            }
        }
    }
    
    gui_mode = 0;
    vga_clear();
}

void shell_loop(void) {
    vga_clear();
    vga_set_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK);
    vga_print("BrackOS v1.7 Shell\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_print("Type 'help' for commands\n\n");
    
    while (1) {
        char path_display[256];
        fs_get_cwd(path_display);
        vga_print(path_display);
        vga_print("> ");
        
        cmd_pos = 0;
        memset(cmd_buffer, 0, CMD_BUFFER_SIZE);
        
        while (1) {
            if (keyboard_has_input()) {
                uint8_t scancode = inb(0x60);
                
                if (scancode == 0x1C) { /* Enter */
                    vga_print("\n");
                    process_command(cmd_buffer);
                    break;
                }
                else if (scancode == 0x0E && cmd_pos > 0) { /* Backspace */
                    cmd_pos--;
                    cmd_buffer[cmd_pos] = 0;
                    vga_print("\b \b");
                }
                else if (scancode < 0x80 && cmd_pos < CMD_BUFFER_SIZE - 1) {
                    static const char scancode_to_ascii[] = {
                        0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
                        '-', '=', 0, 0, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',
                        'o', 'p', '[', ']', 0, 0, 'a', 's', 'd', 'f', 'g', 'h',
                        'j', 'k', 'l', ';', '\'', '`', 0, '\\', 'z', 'x', 'c',
                        'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
                    };
                    
                    if (scancode < sizeof(scancode_to_ascii)) {
                        char c = scancode_to_ascii[scancode];
                        if (c > 0) {
                            cmd_buffer[cmd_pos++] = c;
                            vga_print(&cmd_buffer[cmd_pos - 1]);
                        }
                    }
                }
            }
        }
        
        if (gui_mode) {
            gui_mode_loop();
        }
    }
}

/* IRQ0 handler - installed by IDT */
void irq0_handler(void) {
    timer_irq_handler();
}

void kmain(uint32_t mbi_addr) {
    multiboot_init(mbi_addr);
    vga_init();
    heap_init((void *)0x200000, 2 * 1024 * 1024);  /* 2 MB heap at 0x200000 */
    keyboard_init();
    idt_init();
    interrupts_enable();
    
    /* Initialize timer and RTC */
    timer_init();
    pic_enable_irq(0); /* Enable timer interrupt */
    rtc_init();
    
    recovery_init();
    
    if (fat32_init(0) != 0) {
        vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
        vga_print("Warning: FAT32 init failed (no disk?)\n");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    }
    
    if (recovery_get_mode() == RECOVERY_SAFE_MODE) {
        recovery_safe_mode();
    }
    
    fs_shell_init();
    shell_loop();
    
    __asm__ volatile("cli; hlt");
}
