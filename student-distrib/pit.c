#include "pit.h"

#define SET_FREQUENCY_HIGH ((1193180/30) & 0xFF)
#define SET_FREQUENCY_LOW  ((1193180/30) >> 8)
#define SET_FREQUENCY_PORT 0x40
#define COMMAND_INPUT 0x36
#define COMMAND_PORT 0x43


/* 
 * pit_init
 *   DESCRIPTION: initialize the PIT
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: sets up the PIT's frequency
 */
void pit_init()
{
	outb(COMMAND_INPUT, COMMAND_PORT);              //send command byte          
	
	outb(SET_FREQUENCY_HIGH, SET_FREQUENCY_PORT);   //sets frequency
	outb(SET_FREQUENCY_LOW, SET_FREQUENCY_PORT);

	enable_irq(PIT_INT);
}
