#include "int_handlers.h"

#define LSHIFT 0x2a
#define RSHIFT 0x36
#define LCTRL 0x1d
#define RCTRL 0x1d
#define CAPS 0x3a
#define LALT 0x38
/////////////////////
#define UP   0x48
#define DOWN 0x50
#define ENTER 0x1C
//////////////////////
#define F1 0x3b
#define F2 0x3c
#define F3 0x3d
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
#define KEYBOARD_TYPES 4
#define KEYS 128
//////////////////////
#define MAXHIST 8
#define NUM_TERMINALS 3
int num_hist[NUM_TERMINALS] = {0, 0, 0};
int curr_hist[NUM_TERMINALS] = {0, 0, 0};
int min_hist[NUM_TERMINALS] = {0, 0, 0};
static char modified_hist[NUM_TERMINALS][MAXHIST+1][128];
static char hist[NUM_TERMINALS][MAXHIST+1][128];
static char curr_cmd[NUM_TERMINALS][128];
/////////////////////////////

static unsigned char kbdus[KEYBOARD_TYPES][KEYS] =
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
static int alt = 0;
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
	terminal_write(0, (uint8_t*)"Divide-by-zero\n", DIVTXT);
	khalt(EHALT);
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
	terminal_write(0, (uint8_t*)"Debugger error\n", DEBTXT);
	khalt(EHALT);
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
	terminal_write(0, (uint8_t*)"Non-Maskable Interrupt\n", NMITXT);
	khalt(EHALT);
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
	terminal_write(0, (uint8_t*)"Breakpoint\n", BRTXT);
	khalt(EHALT);
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
	terminal_write(0, (uint8_t*)"Overflow\n", OVTXT);
	khalt(EHALT);
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
	terminal_write(0, (uint8_t*)"Out of bounds\n", BNDTXT);
	khalt(EHALT);
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
	terminal_write(0, (uint8_t*)"Invalid opcode\n", IOPTXT);
	khalt(EHALT);
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
	terminal_write(0, (uint8_t*)"Coprocessor exception\n", COPTXT);
	khalt(EHALT);
}

/* double_fault
 *   DESCRIPTION: out of bounds exception handler, prints error message to screen
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: prints exception message, disables interrupts
 */
void double_fault() { 
	terminal_write(0, (uint8_t*)"Double Fault\n", DBFTXT);
	khalt(EHALT);
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
	terminal_write(0, (uint8_t*)"Coprocessor Segment\n", COSTXT);
	khalt(EHALT);
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
	terminal_write(0, (uint8_t*)"Invalid TSS\n", TSSTXT);
	khalt(EHALT);
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
	terminal_write(0, (uint8_t*)"Segment not present\n", SNPTXT);
	khalt(EHALT);
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
	terminal_write(0, (uint8_t*)"Stack Segment Fault\n", SSFTXT);
	khalt(EHALT);
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
	terminal_write(0, (uint8_t*)"General Protection Fault\n", GPFTXT);
	khalt(EHALT);
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
	int cr2;
	asm ("movl %%cr2,%0"
		:"=r"(cr2) : );
	printf("Page fault! %x\n", cr2);
	khalt(EHALT);
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
	terminal_write(0, (uint8_t*)"Floating point\n", FLPTXT);
	khalt(EHALT);
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
	terminal_write(0, (uint8_t*)"Alignment check\n", ALCTXT);
	khalt(EHALT);
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
	terminal_write(0, (uint8_t*)"Machine Check\n", MCNTXT);
	khalt(EHALT);
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
	terminal_write(0, (uint8_t*)"SIMD Floating Point\n", SIMTXT);
	khalt(EHALT);
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
		int term = get_curr_terminal();
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
		else if (key_scancode == LALT)
			alt = 1;
		else if (key_scancode == (LALT + RELEASE))
			alt = 0;
		else if (key_scancode == UP && curr_hist[term] > min_hist[term]){
			hist_copy_from(modified_hist[term][(curr_hist[term]-1) % (MAXHIST+1)]);
			int i;
			for(i=0;i < 128; i++){
				curr_cmd[term][i] = modified_hist[term][(curr_hist[term]-1) % (MAXHIST+1)] [i];
			}
			curr_hist[term]--;
		}
		else if (key_scancode == DOWN && curr_hist[term] < num_hist[term]){
			hist_copy_from(modified_hist[term][(curr_hist[term]+1) % (MAXHIST+1)]);
			int i;
			for(i=0;i<128;i++){
				curr_cmd[term][i] = modified_hist[term][(curr_hist[term]+1) % (MAXHIST+1)][i];
			}
			curr_hist[term]++;
		}
		else if(key_scancode == ENTER && curr_cmd[term][0] != '\0'){
			int i;
			for(i=0; i<128; i++){
				hist[term][num_hist[term] % (MAXHIST+1)][i] = curr_cmd[term][i];
			}
			curr_hist[term] = ++num_hist[term];

			if(num_hist[term] > MAXHIST)
				min_hist[term]++;

			for(i=0; i <128; i++){
				hist[term][curr_hist[term] % (MAXHIST+1)][i] = '\0';
			}
			int j;
			for(i=0; i < MAXHIST + 1; i++){
				for(j=0; j < 128; j++){
					modified_hist[term][i][j] = hist[term][i][j];
				}
			}

			for(i=0; i < 128; i++)
				curr_cmd[term][i] = '\0';
		}

		if (key_scancode <= SCAN_LIMIT) {
			if (alt == 1 && (key_scancode == F1 || key_scancode == F2 || key_scancode == F3))
				switch_terminals((key_scancode - F1));
			else if (shift == 1 && caps_lock == 0)
			{
				put_char(kbdus[1][key_scancode]);
				if(key_scancode != ENTER){
					curr_cmd[term][strlen(curr_cmd[term])] = kbdus[1][key_scancode];
					modified_hist[term][curr_hist[term] % (MAXHIST+1)][strlen(modified_hist[term][curr_hist[term] % (MAXHIST+1)])] = kbdus[1][key_scancode];
				}
			}
			else if (shift == 0 && caps_lock == 1)
			{
				put_char(kbdus[2][key_scancode]);
				if(key_scancode != ENTER){
					curr_cmd[term][strlen(curr_cmd[term])] = kbdus[2][key_scancode];
					modified_hist[term][curr_hist[term] % (MAXHIST+1)][strlen(modified_hist[term][curr_hist[term] % (MAXHIST+1)])] = kbdus[2][key_scancode];
				}
			}
			else if (shift == 1 && caps_lock == 1)
			{
				put_char(kbdus[3][key_scancode]);
				if(key_scancode != ENTER){
					curr_cmd[term][strlen(curr_cmd[term])] = kbdus[3][key_scancode];
					modified_hist[term][curr_hist[term] % (MAXHIST+1)][strlen(modified_hist[term][curr_hist[term] % (MAXHIST+1)])] = kbdus[3][key_scancode];
				}
			}
			else if (ctrl == 1 && kbdus[0][key_scancode] == 'l')
			{
				clear_terminal();
			}
			else {
				put_char(kbdus[0][key_scancode]);
				if(key_scancode != ENTER){
					curr_cmd[term][strlen(curr_cmd[term])] = kbdus[0][key_scancode];
					modified_hist[term][curr_hist[term] % (MAXHIST+1)][strlen(modified_hist[term][curr_hist[term] % (MAXHIST+1)])] = kbdus[0][key_scancode];
				}
			}			
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
	send_eoi(RTC_INT);
	outb(REG_C, RTC_IDX_PORT);	// read from register c
	inb(RTC_WR_PORT);			// read in, but don't do anything with it
	rtc_flag = 1;	
}

void init_hist(){
	int i, j;
	for(j=0; j < NUM_TERMINALS; j++){
		for(i = 0; i < 128; i++){
			curr_cmd[j][i] = '\0';
		}
	}
	for(j=0; j < NUM_TERMINALS; j++){
		for(i = 0; i <= MAXHIST; i++){
			hist[j][i][0] = '\0';\
		}
	}
}


