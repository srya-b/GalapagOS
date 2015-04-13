#include "int_handlers.h"
#include "rtc.h"
#include "terminal.h"
#include "file_array.h"

#define LSHIFT 0x2a
#define RSHIFT 0x36
#define LCTRL 0x1d
#define RCTRL 0x1d
#define CAPS 0x3a
#define RELEASE 0x80
#define EHALT 0xEF
#define DIVTXT 15
#define DEBTXT 15
#define NMITXT 23
#define BRTXT 11
#define OVTXT 9
#define BNDTXT 14
#define IOPTXT 15
#define COPTXT 22
#define DBFTXT 13
#define COSTXT 20
#define TSSTXT 12
#define SNPTXT 20
#define SSFTXT 20
#define GPFTXT 25
#define PGFTXT 11
#define FLPTXT 15
#define ALCTXT 16
#define MCNTXT 14
#define SIMTXT 20

static unsigned char kbdus[4][128] =
{{
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8',	/* 9 */
  '9', '0', '-', '=', '\b',	/* Backspace */
  '\t',			/* Tab */ 'q', 'w', 'e', 'r',	/* 19 */'t', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',	/* Enter key */ 0, /* 29   - Control */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',	/* 39 */ '\'', '`',   0,		/* Left shift */
 '\\', 'z', 'x', 'c', 'v', 'b', 'n',			/* 49 */ 'm', ',', '.', '/',   0,				/* Right shift */
  '*',  0,	/* Alt */ ' ',	/* Space bar */ 0,	/* Caps lock */ 0,	/* 59 - F1 key ... > */ 0,   0,   0,   0,   0,   0,   0,   0,
    0,	/* < ... F10 */ 0,	/* 69 - Num lock*/ 0,	/* Scroll Lock */ 0,	/* Home key */ 0,	/* Up Arrow */
    0,	/* Page Up */ '-', 0,	/* Left Arrow */ 0, 0,	/* Right Arrow */ '+',  0,	/* 79 - End key*/ 0,	/* Down Arrow */
    0,	/* Page Down */ 0,	/* Insert Key */ 0,	/* Delete Key */ 0,   0,   0, 0,	/* F11 Key */ 0,	/* F12 Key */
    0,	/* All other keys are undefined */
}, {
	0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
	'\t',
	'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}',  '\n', 0 /* L-ctrl */,
	'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '\"', '~', 0 /* L-shift */,
	'|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0 /* R-shift */,
	'*', 0 /* alt */, ' ', 0, 
    0,	/* Caps lock */ 0,	/* 59 - F1 key ... > */ 0,   0,   0,   0,   0,   0,   0,   0, 0,	/* < ... F10 */
    0,	/* 69 - Num lock*/ 0,	/* Scroll Lock */ 0,	/* Home key */ 0,	/* Up Arrow */ 0,	/* Page Up */ '-', 0,	/* Left Arrow */
    0, 0,	/* Right Arrow */ '+', 0,	/* 79 - End key*/ 0,	/* Down Arrow */ 0,	/* Page Down */ 0,	/* Insert Key */ 0,	/* Delete Key */
    0,   0,   0, 0,	/* F11 Key */  0,	/* F12 Key */ 0,	/* All other keys are undefined */
}, {
	 0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', '\t',
	'Q', 'W', 'E', 'R','T', 'Y', 'U', 'I', 'O', 'P', '[', ']', '\n', 0 /* L-ctrl */, 'A', 'S',
	'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', '\'', '`', 0,'\\', 'Z', 'X', 'C', 'V',
	'B', 'N', 'M', ',', '.', '/', 0, '*', 0, ' ', 0,
    0,	/* Caps lock */ 0,	/* 59 - F1 key ... > */ 0,   0,   0,   0,   0,   0,   0,   0, 0,	/* < ... F10 */
    0,	/* 69 - Num lock*/ 0,	/* Scroll Lock */ 0,	/* Home key */ 0,	/* Up Arrow */ 0,	/* Page Up */ '-', 0,	/* Left Arrow */
    0, 0,	/* Right Arrow */ '+', 0,	/* 79 - End key*/ 0,	/* Down Arrow */ 0,	/* Page Down */ 0,	/* Insert Key */ 0,	/* Delete Key */
    0,   0,   0, 0,	/* F11 Key */  0,	/* F12 Key */ 0,	/* All other keys are undefined */
}, {
	0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
	'\t', 'q', 'w', 'e', 'r',	/* 19 */'t', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',	/* Enter key */ 0, /* 29   - Control */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ':', '\"', '~', 0 /* L-shift */,
	'|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0 /* R-shift */,
	'*', 0 /* alt */, ' ', 0,  0,	/* Caps lock */ 0,	/* 59 - F1 key ... > */ 0,   0,   0,   0,   0,   0,   0,   0, 0,	/* < ... F10 */
    0,	/* 69 - Num lock*/ 0,	/* Scroll Lock */ 0,	/* Home key */ 0,	/* Up Arrow */ 0,	/* Page Up */ '-', 0,	/* Left Arrow */
    0, 0,	/* Right Arrow */ '+', 0,	/* 79 - End key*/ 0,	/* Down Arrow */ 0,	/* Page Down */ 0,	/* Insert Key */ 0,	/* Delete Key */
    0,   0,   0, 0,	/* F11 Key */  0,	/* F12 Key */ 0,	/* All other keys are undefined */

}};

static int caps_lock = 0;
static int shift = 0;
static int ctrl = 0;
volatile uint8_t rtc_flag = 0;

/************** EXCEPTIONS ********************/

/* 
 * divide_by_zero
 *   DESCRIPTION: Divide by zero exception handler, prints error message to screen
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: prints exception message, disables interrupts
 */

void divide_by_zero() { 
	//clear();
	//printf("Divide-by-zero exception!\n");
	terminal_write(DIVTXT, (uint8_t*)"Divide-by-zero\n");
	khalt(EHALT);
	//while(1) { }
}

/* 
 * debugger
 *   DESCRIPTION: Debugger exception handler, prints error message to screen
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: prints exception message, disables interrupts
 */
void debugger() {
	//clear();
	//printf("Debugger exception!\n");
	terminal_write(DEBTXT, (uint8_t*)"Debugger error\n");
	khalt(EHALT);
	// cli();
	// while(1) { }
}

/* 
 * NMI
 *   DESCRIPTION: Non-maskable interrupts exception handler, prints error message to screen
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: prints exception message, disables interrupts
 */
void NMI() {
	//clear(); 
	//printf("Non-Maskable Interrupt!\n");
	terminal_write(NMITXT, (uint8_t*)"Non-Maskable Interrupt\n");
	khalt(EHALT);
	// cli();
	// while(1) { }
}

/* 
 * breakpoint
 *   DESCRIPTION: breakpoint exception handler, prints error message to screen
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: prints exception message, disables interrupts
 */
void breakpoint() { 
	//clear();
	//printf("Breakpoint!\n");
	terminal_write(BRTXT, (uint8_t*)"Breakpoint\n");
	khalt(EHALT);
	// cli();
	// while(1) { }
}

/* 
 * overflow
 *   DESCRIPTION: overflow exception handler, prints error message to screen
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: prints exception message, disables interrupts
 */
void overflow() { 
	//clear();
	//printf("Overflow!\n");
	terminal_write(OVTXT, (uint8_t*)"Overflow\n");
	khalt(EHALT);
	// cli();
	// while(1) { }
}

/* 
 * bounds
 *   DESCRIPTION: out of bounds exception handler, prints error message to screen
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: prints exception message, disables interrupts
 */
void bounds() { 
	//clear();
	//printf("Out of bounds!\n");
	terminal_write(BNDTXT, (uint8_t*)"Out of bounds\n");
	khalt(EHALT);
	// cli();
	// while(1) { }
}

/* 
 * invalid_opcode
 *   DESCRIPTION: invalid opcode exception handler, prints error message to screen
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: prints exception message, disables interrupts
 */
void invalid_opcode() { 
	//clear();
	//printf("Invalid opcode!\n");
	terminal_write(IOPTXT, (uint8_t*)"Invalid opcode\n");
	khalt(EHALT);
	// cli();
	// while(1) { }
}

/* 
 * coprocessor
 *   DESCRIPTION: coprocessor exception handler, prints error message to screen
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: prints exception message, disables interrupts
 */
void coprocessor() { 
	//clear();
	//printf("Coprocessor exception!\n");
	terminal_write(COPTXT, (uint8_t*)"Coprocessor exception\n");
	khalt(EHALT);
	// cli();
	// while(1) { }
}

/* double_fault
 *   DESCRIPTION: out of bounds exception handler, prints error message to screen
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: prints exception message, disables interrupts
 */
void double_fault() { 
	terminal_write(DBFTXT, (uint8_t*)"Double Fault\n");
	khalt(EHALT);
	//clear();
	//printf("Double fault!\n");
	//cli();
	//while(1) { }
}

/* 
 * coproccesor_segment
 *   DESCRIPTION: coprocessor_segment exception handler, prints error message to screen
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: prints exception message, disables interrupts
 */
void coprocessor_segment() { 
	terminal_write(COSTXT, (uint8_t*)"Coprocessor Segment\n");
	khalt(EHALT);
	// clear();
	// printf("Coprocessor segment exception!\n");
	// cli();
	// while(1) { }
}

/* 
 * invalid_tss
 *   DESCRIPTION: invalid_tss exception handler, prints error message to screen
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: prints exception message, disables interrupts
 */
void invalid_tss() { 
	terminal_write(TSSTXT, (uint8_t*)"Invalid TSS\n");
	khalt(EHALT);
	// clear();
	// printf("Invalid TSS!\n");
	// cli();
	// while(1) { }
}

/* 
 * segment_not_present
 *   DESCRIPTION: segment not present exception handler, prints error message to screen
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: prints exception message, disables interrupts
 */
void segment_not_present() { 
	terminal_write(SNPTXT, (uint8_t*)"Segment not present\n");
	khalt(EHALT);
	// clear();
	// printf("Segment not present!\n");
	// cli();
	// while(1) { }
}

/* 
 * stack_segment_fault
 *   DESCRIPTION: stack segment fault exception handler, prints error message to screen
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: prints exception message, disables interrupts
 */
void stack_segment_fault() { 
	terminal_write(SSFTXT, (uint8_t*)"Stack Segment Fault\n");
	khalt(EHALT);
	// clear();
	// printf("Stack segment fault!\n");
	// cli();
	// while(1) { }
}

/* 
 * general_protecton_fault
 *   DESCRIPTION: general protection fault exception handler, prints error message to screen
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: prints exception message, disables interrupts
 */
void general_protection_fault() {
	terminal_write(GPFTXT, (uint8_t*)"General Protection Fault\n");
	khalt(EHALT);
	// clear();
	// printf("General protection fault!\n");
	// cli();
	// while(1) { }
}

/* 
 * page_fault
 *   DESCRIPTION: page fault exception handler, prints error message to screen
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: prints exception message, disables interrupts
 */
void page_fault() {
	//clear();
	// int cr2;
	// asm ("movl %%cr2,%0"
	// 	:"=r"(cr2) : );
	// printf("Page fault! %x\n", cr2);
	terminal_write(PGFTXT, (uint8_t*)"Page Fault\n");
	khalt(EHALT);
	// cli();
	// while(1) { }
}

/* 
 * floating_point
 *   DESCRIPTION: floating point exception handler, prints error message to screen
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: prints exception message, disables interrupts
 */
void floating_point() {
	// clear();
	// printf("Floating point exception!\n");
	terminal_write(FLPTXT, (uint8_t*)"Floating point\n");
	khalt(EHALT);
	// cli();
	// while(1) { }
}

/* 
 * alignment_check
 *   DESCRIPTION: alignment check exception handler, prints error message to screen
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: prints exception message, disables interrupts
 */
void alignment_check() {
	// clear();
	// printf("Alignment check exception!\n");
	terminal_write(ALCTXT, (uint8_t*)"Alignment check\n");
	khalt(EHALT);
	// cli();
	// while(1) { }
}

/* 
 * machine_check
 *   DESCRIPTION: machine check exception handler, prints error message to screen
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: prints exception message, disables interrupts
 */
void machine_check() {
	// clear();
	// printf("Machine check exception!\n");
	terminal_write(MCNTXT, (uint8_t*)"Machine Check\n");
	khalt(EHALT);
	// cli();
	// while(1) { }
}

/* 
 * SIMD_floating_point
 *   DESCRIPTION: SIMD floating point exception handler, prints error message to screen
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: prints exception message, disables interrupts
 */
void SIMD_floating_point() {
	terminal_write(SIMTXT, (uint8_t*)"SIMD Floating Point\n");
	khalt(EHALT);
	// clear();
	// printf("SIMD floating point exception!\n");
	// cli();
	// while(1) { }
}

/****************** INTERRUPTS ******************/

/* 
 * keyboard_int
 *   DESCRIPTION: keyboard interrupt handler, prints error message to screen
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: prints key pressed to screen (reads from keyboard port)
 */
void keyboard_int() {
    send_eoi(KEYBOARD_INT);
		int key_scancode = inb(KEYBOARD_PORT);
		if (key_scancode == LSHIFT || key_scancode == RSHIFT)
			shift = 1;
		else if (key_scancode == (LSHIFT + RELEASE) || key_scancode == (RSHIFT + RELEASE))
			shift = 0;
		else if (key_scancode == LCTRL)
			ctrl = 1;
		else if (key_scancode == (LCTRL + RELEASE))
			ctrl = 0;
		else if (key_scancode == CAPS && caps_lock == 0)
			caps_lock = 1;
		else if (key_scancode == CAPS && caps_lock == 1)
			caps_lock = 0;

		if (key_scancode <= SCAN_LIMIT) {
			if (shift == 1 && caps_lock == 0)
			{
				put_char(kbdus[1][key_scancode]);
			}
			else if (shift == 0 && caps_lock == 1)
			{
				put_char(kbdus[2][key_scancode]);
			}
			else if (shift == 1 && caps_lock == 1)
			{
				put_char(kbdus[3][key_scancode]);
			}
			else if (ctrl == 1 && kbdus[0][key_scancode] == 'l')
			{
				clear_terminal();
			}
			else 
				put_char(kbdus[0][key_scancode]);
		}


}

/* 
 * rtc_int
 *   DESCRIPTION: real time clock interrupt handler, prints error message to screen
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: prints interrupt message, reads from Register C to allow more RTC ints to come in 
 */
void rtc_int() {
	outb(REG_C, RTC_IDX_PORT);	// read from register c
	inb(RTC_WR_PORT);			// read in, but don't do anything with it
	rtc_flag = 1;	
	send_eoi(RTC_INT);
}
