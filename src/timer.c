#include "timer.h"
#include "io.h"
#include "rtc.h"

timer_t system_timer = {0, 0, 0};

/* Initialize PIT (Programmable Interval Timer) for ~100Hz */
void timer_init(void) {
    /* Program PIT channel 0 for square wave, binary mode */
    /* Command: channel 0, both bytes, square wave, binary */
    outb(PIT_COMMAND, 0x34);
    
    /* Send reload value (low byte first, then high byte) */
    /* This sets frequency to ~100 Hz (10ms per tick) */
    outb(PIT_CHANNEL0, PIT_RELOAD & 0xFF);
    outb(PIT_CHANNEL0, (PIT_RELOAD >> 8) & 0xFF);
}

/* IRQ0 handler - called by IDT on timer interrupt */
void timer_irq_handler(void) {
    system_timer.ticks++;
    system_timer.milliseconds += 10; /* Each tick is ~10ms */
    
    /* Update system uptime once a second (~100 ticks at 100 Hz).
     * A small wraparound counter, not "% 100" on the 64-bit tick count,
     * so this needs no 64-bit divide (unavailable in hardware on i386). */
    system_timer.tick_in_second++;
    if (system_timer.tick_in_second >= 100) {
        system_timer.tick_in_second = 0;
        rtc_update_uptime();
    }
}

/* Get number of ticks since boot */
uint64_t timer_get_ticks(void) {
    return system_timer.ticks;
}

/* Get milliseconds since boot */
uint32_t timer_get_milliseconds(void) {
    return system_timer.milliseconds;
}

/* Sleep for specified milliseconds */
void timer_sleep_ms(uint32_t ms) {
    uint32_t target = system_timer.milliseconds + ms;
    while (system_timer.milliseconds < target);
}

/* Sleep for specified ticks */
void timer_sleep_ticks(uint64_t ticks) {
    uint64_t target = system_timer.ticks + ticks;
    while (system_timer.ticks < target);
}
