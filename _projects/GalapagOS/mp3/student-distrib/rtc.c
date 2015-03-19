#include "rtc.h"

/*
 *	rtc_init
 *		DESCRIPTION:  Turns on the RTC and enables periodic
 *					  interrupts.
 *		INPUTS: none
 *		OUTPUTS: none
 *		RETURN VALUE: none
 *		SIDE EFFECTS:  Turns on the RTC.
 */

void rtc_init(){
	/* Select Register B of the RTC*/
    outb(RTC_REG_B, RTC_IDX_PORT);

    /*Store current value of register B*/
    char old_regb = inb(RTC_WR_PORT);

    /*Select Register B of the RTC again to send info*/
	outb(RTC_REG_B, RTC_IDX_PORT);

	/*Set the enable bit in register B and send it back*/
    outb( old_regb | RTC_EN, RTC_WR_PORT);
    
    return;
}

