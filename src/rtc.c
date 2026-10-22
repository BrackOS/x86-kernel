#include "rtc.h"
#include "io.h"
#include "vga.h"

uptime_t system_uptime = {0, 0, 0, 0};
static rtc_time_t last_time = {0};
static uint8_t rtc_initialized = 0;

/* Read byte from CMOS */
static uint8_t cmos_read(uint8_t addr) {
    outb(RTC_ADDR_PORT, addr);
    return inb(RTC_DATA_PORT);
}

/* Write byte to CMOS */
static void cmos_write(uint8_t addr, uint8_t value) {
    outb(RTC_ADDR_PORT, addr);
    outb(RTC_DATA_PORT, value);
}

/* BCD to Binary */
uint8_t bcd_to_bin(uint8_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

/* Binary to BCD */
uint8_t bin_to_bcd(uint8_t bin) {
    return ((bin / 10) << 4) | (bin % 10);
}

/* Check if RTC is updating */
int rtc_is_updating(void) {
    return (cmos_read(RTC_STATUS_A) & 0x80) != 0;
}

/* Initialize RTC */
void rtc_init(void) {
    if (rtc_initialized) return;
    
    vga_print("RTC: Initializing...\n");
    
    /* Wait for update to finish */
    while (rtc_is_updating());
    
    /* Read initial time */
    rtc_read_time(&last_time);
    
    vga_print("RTC: ");
    vga_print_int(last_time.year);
    vga_print("-");
    vga_print_int(last_time.month);
    vga_print("-");
    vga_print_int(last_time.day);
    vga_print(" ");
    vga_print_int(last_time.hour);
    vga_print(":");
    vga_print_int(last_time.minute);
    vga_print(":");
    vga_print_int(last_time.second);
    vga_print("\n");
    
    rtc_initialized = 1;
}

/* Enable RTC periodic interrupt */
void rtc_enable_irq(void) {
    uint8_t status_b = cmos_read(RTC_STATUS_B);
    cmos_write(RTC_STATUS_B, status_b | 0x40); /* Enable periodic interrupt */
}

/* Read current time from RTC */
rtc_time_t rtc_get_time(void) {
    rtc_time_t time;
    rtc_read_time(&time);
    return time;
}

/* Read time with BCD conversion */
void rtc_read_time(rtc_time_t *time) {
    /* Wait for RTC update to complete */
    while (rtc_is_updating());
    
    uint8_t status_b = cmos_read(RTC_STATUS_B);
    int is_binary = (status_b & 0x04) != 0;
    int is_24h = (status_b & 0x02) != 0;
    
    /* Read all registers */
    uint8_t second = cmos_read(RTC_SECONDS);
    uint8_t minute = cmos_read(RTC_MINUTES);
    uint8_t hour = cmos_read(RTC_HOURS);
    uint8_t day_of_week = cmos_read(RTC_DAY_OF_WEEK);
    uint8_t day = cmos_read(RTC_DAY_OF_MONTH);
    uint8_t month = cmos_read(RTC_MONTH);
    uint8_t year = cmos_read(RTC_YEAR);
    
    /* Convert from BCD if needed */
    if (!is_binary) {
        second = bcd_to_bin(second);
        minute = bcd_to_bin(minute);
        hour = bcd_to_bin(hour & 0x7F);
        day = bcd_to_bin(day);
        month = bcd_to_bin(month);
        year = bcd_to_bin(year);
    }
    
    /* Handle 12-hour format */
    if (!is_24h && (hour & 0x80)) {
        hour = ((hour & 0x7F) + 12) % 24;
    }
    
    /* Set century (assuming 20xx) */
    uint16_t full_year = 2000 + year;
    
    time->second = second;
    time->minute = minute;
    time->hour = hour;
    time->day_of_week = day_of_week;
    time->day = day;
    time->month = month;
    time->year = full_year;
}

/* Update system uptime (called periodically by timer IRQ) */
void rtc_update_uptime(void) {
    system_uptime.seconds++;
    
    if (system_uptime.seconds >= 60) {
        system_uptime.seconds = 0;
        system_uptime.minutes++;
    }
    
    if (system_uptime.minutes >= 60) {
        system_uptime.minutes = 0;
        system_uptime.hours++;
    }
    
    if (system_uptime.hours >= 24) {
        system_uptime.hours = 0;
        system_uptime.days++;
    }
}

/* Get system uptime */
uptime_t rtc_get_uptime(void) {
    return system_uptime;
}
