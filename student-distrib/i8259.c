 /* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

/* Interrupt masks to determine which interrupts
 * are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7 */
uint8_t slave_mask; /* IRQs 8-15 */

#define MASK_ALL 0xFF

/*
 *	i8259_init
 *		DESCRIPTION:  Turns on the PIC and enables recieving of 
 *					  interrupts
 *		INPUTS: none
 *		OUTPUTS: none
 *		RETURN VALUE: none
 *		SIDE EFFECTS:  Turns on the PIC.
 */
void
i8259_init(void)
{
    /* init master and slave masks */
	master_mask = MASK_ALL;
	slave_mask = MASK_ALL;

    /* send ICW1 to master, ICW2 to master and slave, etc. */
	outb(ICW1, MASTER_8259_PORT);
	outb(ICW1, SLAVE_8259_PORT);

	outb(ICW2_MASTER, MASTER_8259_PORT+1);
	outb(ICW2_SLAVE, SLAVE_8259_PORT+1);

	outb(ICW3_MASTER, MASTER_8259_PORT+1);
	outb(ICW3_SLAVE, SLAVE_8259_PORT+1);

	outb(ICW4, MASTER_8259_PORT+1);
	outb(ICW4, SLAVE_8259_PORT+1);

    /* send mask to PIC ports */
	outb(master_mask, MASTER_8259_PORT+1);
	outb(slave_mask, SLAVE_8259_PORT+1);

}

/*
 *	enable_irq
 *		DESCRIPTION:  Enables an IRQ line for interrupts on the
 *					  master or slave depending on the line.
 *		INPUTS: irq_num - IRQ line number to enable
 *		OUTPUTS: none
 *		RETURN VALUE: none
 *		SIDE EFFECTS:  Enables interrupts from specified IRQ line.
 */
void
enable_irq(uint32_t irq_num)
{
	/*Need to change the index in case IRQ is on the slave*/
	char irq_index = irq_num % NUM_IRQ_LINES;

	/*Mask that represents the line the irq is on*/
	char adjust_mask = (0x1 << irq_index);

	/*If the IRQ is on the master we send to the master*/
	if (irq_num < NUM_IRQ_LINES) {
		master_mask &= (~adjust_mask);
		outb(inb(MASTER_8259_PORT+1) & (~adjust_mask), MASTER_8259_PORT+1);
	} 

	/*If more than 8 then the interrupt is on the slave*/
	else {
		slave_mask &= (~adjust_mask);
		outb(inb(SLAVE_8259_PORT+1) & (~adjust_mask), SLAVE_8259_PORT+1);
	}
}

/*
 *	disable_irq
 *		DESCRIPTION:  Disables the IRQ line specified on the 
 *					  master or the slave.
 *		INPUTS: irq_num - IRQ line number to disable
 *		OUTPUTS: none
 *		RETURN VALUE: none
 *		SIDE EFFECTS:  Disables interrupts from specified IRQ line.
 */
void
disable_irq(uint32_t irq_num)
{
	char mask=0x01;

	/*Check if the IRQ line is on the master*/
	if(irq_num < NUM_IRQ_LINES)
	{
		/*Set the IRQ mask to send to the master*/
		mask = (mask << irq_num);
		master_mask = master_mask | mask;
		outb(inb(MASTER_8259_PORT+1) | mask, MASTER_8259_PORT+1);
	}
	
	/*Check if the IRQ line is on the slave*/
	else if(irq_num >= NUM_IRQ_LINES && irq_num < 2*NUM_IRQ_LINES)
	{
		/*Set the IRQ mask to send to the slave*/
		mask = (mask << (irq_num - NUM_IRQ_LINES));
		slave_mask=slave_mask | mask;
 		outb(inb(SLAVE_8259_PORT+1) | mask, SLAVE_8259_PORT+1);
	}

}

/* Send end-of-interrupt signal for the specified IRQ */
/*
 *	send_eoi
 *		DESCRIPTION:  Sends end-of-interrupt signal for specified
 *					  IRQ line.
 *		INPUTS: irq_num - IRQ line that is finished being serviced
 *		OUTPUTS: none
 *		RETURN VALUE: none
 *		SIDE EFFECTS:  PIC becomes ready to recieve interrupts again
 */
void
send_eoi(uint32_t irq_num)
{
    /* send to both if irq on slave */
    if (irq_num >= NUM_IRQ_LINES)
    {
    	/*Send eoi to slave*/
     	outb(EOI | (unsigned char)(irq_num % NUM_IRQ_LINES), SLAVE_8259_PORT);		
    	
     	/*Send eoi to disable the slave port on the master*/
    	outb(EOI | (unsigned char)(0x2), MASTER_8259_PORT);
    }

    /* Send to master no matter what */
    else outb(EOI | (unsigned char)(irq_num % NUM_IRQ_LINES), MASTER_8259_PORT);
}

