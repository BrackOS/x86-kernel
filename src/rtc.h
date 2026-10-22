#ifndef RTC_H
#define RTC_H

#include <stdint.h>

/* CMOS RTC ports */
#define RTC_ADDR_PORT   0x70
#define RTC_DATA_PORT   0x71

/* CMOS register addresses */
#define RTC_SECONDS     0x00
#define RTC_MINUTES     0x02
#define RTC_HOURS       0x04
#define RTC_DAY_OF_WEEK 0x06
#define RTC_DAY_OF_MONTH 0x07
#define RTC_MONTH       0x08
#define RTC_YEAR        0x09
#define RTC_STATUS_A    0x0A
#define RTC_STATUS_B    0x0B

/* Date/time structure */
typedef struct {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day_of_week;
    uint8_t day;
    uint8_t month;
    uint16_t year;
} rtc_time_t;

/* System uptime */
typedef struct {
    uint32_t days;
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
} uptime_t;

/* Initialization and status */
void rtc_init(void);
void rtc_enable_irq(void);
int rtc_is_updating(void);

/* Read time */
rtc_time_t rtc_get_time(void);
void rtc_read_time(rtc_time_t *time);

/* BCD conversion */
uint8_t bcd_to_bin(uint8_t bcd);
uint8_t bin_to_bcd(uint8_t bin);

/* System uptime tracking */
void rtc_update_uptime(void);
uptime_t rtc_get_uptime(void);

/* Global uptime counter */
extern uptime_t system_uptime;

#endif
