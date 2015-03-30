#ifndef _RTC_H
#define _RTC_H

#include "lib.h"
#include "types.h"

/*Write and read ports for the RTC*/
#define RTC_IDX_PORT 0x70
#define RTC_WR_PORT 0x71
#define RTC_REG_B 0x8B
#define RTC_EN 0x40
#define RTC_REG_A 0x8A

/* open rtc, init to 2 Hz */
int rtc_open();

/* wait until next rtc interrupt */
int rtc_read();

/* write a new frequency to the rtc */
int rtc_write(uint32_t frequency);

/* closes the rtc */
int rtc_close();

/*Enables the rtc and periodic interrupts*/
void rtc_init();

#endif
