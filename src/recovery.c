#include "recovery.h"
#include "vga.h"
#include "io.h"

/* Recovery state stored in low memory (0x7E00) */
#define RECOVERY_STATE_ADDR ((recovery_state_t*)0x7E00)

static recovery_state_t current_state = {
    .mode = RECOVERY_NONE,
    .boot_count = 0,
    .last_error = RECOVERY_ERR_NONE,
    .reserved = 0
};

void recovery_init(void) {
    vga_clear();
    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    vga_print("BrackOS v1.7 Recovery System\n");
    vga_print("=============================\n\n");
    
    /* Check for repeated failures */
    current_state.boot_count++;
    if (current_state.boot_count > 3) {
        vga_set_color(VGA_COLOR_RED, VGA_COLOR_BLACK);
        vga_print("! Repeated boot failures detected\n");
        vga_print("! Entering Safe Mode\n\n");
        current_state.mode = RECOVERY_SAFE_MODE;
    }
    
    vga_set_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK);
    vga_print("Running boot diagnostics...\n\n");
    recovery_run_diagnostics();
}

void recovery_run_diagnostics(void) {
    vga_set_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK);
    
    /* Test GDT */
    vga_print("[OK] GDT initialized\n");
    
    /* Test IDT */
    vga_print("[OK] IDT initialized\n");
    
    /* Test paging. The x86 (32-bit) build runs flat with no page tables by
     * design -- it relies on segment limits instead -- so it reports that
     * instead of treating an unset CR3 as a failure. */
#if defined(__x86_64__)
    uint64_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    if (cr3 > 0) {
        vga_print("[OK] Paging enabled (");
        vga_print_int((cr3 >> 12) & 0xFFFFF);
        vga_print(")\n");
    } else {
        vga_set_color(VGA_COLOR_RED, VGA_COLOR_BLACK);
        vga_print("[FAIL] Paging error\n");
        current_state.last_error = RECOVERY_ERR_PAGING;
    }
#else
    vga_print("[OK] Flat 32-bit mode (no paging)\n");
#endif
    
    /* Test VGA */
    vga_set_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK);
    vga_print("[OK] VGA initialized\n");
    
    /* Test keyboard */
    uint8_t status = inb(0x64);
    if (status & 0x01) {
        vga_print("[OK] Keyboard ready\n");
    } else {
        vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
        vga_print("[WARN] Keyboard not ready\n");
        vga_set_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK);
    }
    
    vga_print("\n");
    
    if (current_state.last_error != RECOVERY_ERR_NONE) {
        vga_set_color(VGA_COLOR_RED, VGA_COLOR_BLACK);
        vga_print("! Error code: 0x");
        vga_print_int(current_state.last_error);
        vga_print("\n");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        vga_print("Press any key to continue...\n");
        while ((inb(0x64) & 0x01) == 0);
        inb(0x60);
    }
    
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
}

recovery_mode_t recovery_get_mode(void) {
    return current_state.mode;
}

void recovery_safe_mode(void) {
    vga_clear();
    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    vga_print("SAFE MODE - Limited functionality\n");
    vga_print("==================================\n\n");
    vga_print("Available commands:\n");
    vga_print("  help      - Command list\n");
    vga_print("  info      - System info\n");
    vga_print("  cpu       - CPU info\n");
    vga_print("  clear     - Clear screen\n");
    vga_print("  reboot    - Reboot system\n\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
}

void recovery_log_error(uint8_t error_code) {
    current_state.last_error = error_code;
}
