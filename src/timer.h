#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

/* PIT (Programmable Interval Timer) ports */
#define PIT_CHANNEL0    0x40
#define PIT_CHANNEL1    0x41
#define PIT_CHANNEL2    0x42
#define PIT_COMMAND     0x43

/* PIT frequency and reload value */
#define PIT_FREQUENCY   1193182  /* Base frequency in Hz */
#define PIT_RELOAD      11932    /* ~100 Hz = 10ms ticks */

/* Timer state */
typedef struct {
    uint64_t ticks;
    uint32_t milliseconds;
    uint32_t tick_in_second;   /* 0-99; counts up to a full second at ~100 Hz */
} timer_t;

/* Initialization */
void timer_init(void);
void timer_irq_handler(void);

/* Reading timer state */
uint64_t timer_get_ticks(void);
uint32_t timer_get_milliseconds(void);

/* Sleep functions */
void timer_sleep_ms(uint32_t ms);
void timer_sleep_ticks(uint64_t ticks);

/* Global timer */
extern timer_t system_timer;

#endif
