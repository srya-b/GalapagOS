#ifndef _RTC_H
#define _RTC_H

#include "lib.h"

/*Write and read ports for the RTC*/
#define RTC_IDX_PORT 0x70
#define RTC_WR_PORT 0x71
#define RTC_REG_B 0x8B
#define RTC_EN 0x40


/*Enables the rtc and periodic interrupts*/
void rtc_init();

#endif
