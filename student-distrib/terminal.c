#include "terminal.h"

#define VID_MEM 0xb8000
#define INACTIVE_VID_MEM 0xb7000
#define TERM1 0xb9000
#define TERM2 0xba000
#define TERM3 0xbb000
#define WIDTH 80
#define HEIGHT 25
#define INDEX(x,y) (x + (WIDTH * y))
#define BLACK 0
#define WHITE 15
#define GREEN 2 
#define CYAN 3
#define YELLOW 0xe
#define ASCII_SPACE 32
#define BACKSPACE 0x08
#define TAB 0x09
#define DELETE 0x7f
#define NEW_LINE 0xa
#define BUFFER_LIMIT 128
#define CURSOR_LOCATION_HIGH 0x0E
#define CURSOR_LOCATION_LOW 0x0F
#define VGA_ADDRESS_REGISTER 0x3D4
#define VGA_DATA_REGISTER 0x3D5
#define BYTE_SHIFT 8
#define LOW_BYTE_MASK 0xFF
#define NUM_TERMINALS 3

static unsigned short *mem;
static screen_char_t blank[NUM_TERMINALS];

/* New for virt terminals */
static int curr_terminal;
static int linelength[NUM_TERMINALS];
static int lastlinelength[NUM_TERMINALS];
static int linestart[NUM_TERMINALS];
static uint8_t linebuffer[NUM_TERMINALS][BUFFER_LIMIT+1];
static uint8_t lastlinebuffer[NUM_TERMINALS][BUFFER_LIMIT+1];
static volatile int rd[NUM_TERMINALS];
static int curr_x[NUM_TERMINALS];
static int curr_y[NUM_TERMINALS];

/*
 * scroll_line
 * DESCRIPTION: scrolls a terminal
 * INPUTS: terminal number, wheter showing terminal
 * OUTPUTS: none
 * RETURN VALIE: none
 */
void scroll_line(int t, int user_triggered)
{
	int terminal = get_curr_process_terminal();
	terminal = t;
	unsigned short int character;
	screen_char_t *ptr = (screen_char_t*)(&character);
	ptr->character = ' ';
	
	if (terminal == 0)
		ptr->fore = YELLOW;
	else if (terminal == 1)
		ptr->fore = GREEN;
	else if (terminal == 2)
		ptr->fore = CYAN;
		
	ptr->back = BLACK;

	int i = curr_y[terminal] - HEIGHT + 1;
	if (user_triggered) {
		memcpy((void*)INACTIVE_VID_MEM, (const void*)(INACTIVE_VID_MEM + i*WIDTH*2), (HEIGHT - i) * WIDTH * 2);
		memset_word((void*)(INACTIVE_VID_MEM + 2*(HEIGHT - i)*WIDTH), character, WIDTH);

	} else {
		memcpy(mem, mem + i*WIDTH, (HEIGHT - i) * WIDTH * 2);
		memset_word(mem + (HEIGHT - i)*WIDTH, character, WIDTH);
	}
	curr_y[terminal] = HEIGHT - 1;
}

/*
 * clear_buffer
 * DESCRIPTION: clears a the curr terminal buffer
 * INPUTS: none
 * OUTPUTS: none
 * RETURN VALUE: none
 */
void clear_buffer()
{
	int terminal = get_curr_process_terminal();
	int i;
	for (i = 0; i < BUFFER_LIMIT; i++)
		linebuffer[terminal][i] = '\0';
}

/*
 * put_char
 * DESCRIPTION: acts on keyboard pressed keys
 * INPUTS: character from keyboard (printable character)
 * OUTPUTS: none
 * SIDE EFFECTS: shows keyboard-pressed character on screen
 * RETURN VALIE: none
 */
void put_char(char c)
{
	int terminal = get_curr_process_terminal();
	terminal = curr_terminal;
	screen_char_t *data = (screen_char_t*)((short int*)INACTIVE_VID_MEM + (INDEX(curr_x[terminal], curr_y[terminal])));
	cli();
	if (linelength[terminal] < BUFFER_LIMIT)
	{
		if (c == '\t') 
		{
			int i;
			curr_x[terminal] += 8;
		 	for (i = 0; i < 8; i++)
		 	{
				linebuffer[terminal][linelength[terminal]] = ' ';
				linelength[terminal] += 1;
		 	}
			update_cursor(curr_y[terminal], curr_x[terminal]);
		}		 	
		if (curr_x[terminal] >= WIDTH)
		{
			curr_x[terminal] = 0;
			curr_y[terminal] += 1;
			update_cursor(curr_y[terminal], curr_x[terminal]);
		}
		if (c >= ' ') {
			linebuffer[terminal][linelength[terminal]] = c;
			data->character = c;
			if (terminal == 0)
				data->fore = YELLOW;
			else if (terminal == 1)
				data->fore = GREEN;
			else if (terminal == 2)
				data->fore = CYAN;
			data->back = BLACK;
			curr_x[terminal] += 1;
			linelength[terminal] += 1;
			update_cursor(curr_y[terminal], curr_x[terminal]);
		} 
	}
		if (c == BACKSPACE) {
			if (linelength[terminal] > 0) curr_x[terminal] -= 1;
			data = (screen_char_t*)(INACTIVE_VID_MEM + INDEX(curr_x[terminal], curr_y[terminal])*2);
			*data = blank[terminal];
			if (linelength[terminal] > 0) linelength[terminal] -= 1;
			linebuffer[terminal][linelength[terminal]] = '\0';
			update_cursor(curr_y[terminal], curr_x[terminal]);
		}
	
	if (c == '\n' ) {
		linebuffer[terminal][linelength[terminal]] = c;
		linelength[terminal] += 1;
		lastlinelength[terminal] = linelength[terminal];
		linelength[terminal] = 0;
		linestart[terminal] = 0;
		
		int i;
		for (i = 0; i < lastlinelength[terminal]; i++)
		{
			lastlinebuffer[terminal][i] = linebuffer[terminal][i];
			linebuffer[terminal][i] = '\0';
		}
		curr_x[terminal] = 0;
		curr_y[terminal] += 1;
		rd[terminal] = 1;

		update_cursor(curr_y[terminal], curr_x[terminal]);
	}	

	if (curr_y[terminal] >= HEIGHT)
	{
		scroll_line(terminal, 1);
		curr_y[terminal] = HEIGHT - 1;
		update_cursor(curr_y[terminal], curr_x[terminal]);
	}
	sti();
}

/*
 * put_char_out
 * DESCRIPTION: acts on characters from terminal_write
 * INPUTS: character from program
 * OUTPUTS: none
 * SIDE EFFECTS: shows terminal_write character on screen
 * RETURN VALIE: none
 */
void put_char_out(char c)
{
	int terminal = get_curr_process_terminal();
	screen_char_t *data = (screen_char_t*)(mem + INDEX(curr_x[terminal], curr_y[terminal]));

	if (curr_x[terminal] >= WIDTH)
	{
		linestart[terminal] = 0;
		curr_x[terminal] = 0;
		curr_y[terminal] += 1;
		update_cursor(curr_y[terminal], curr_x[terminal]);
	}

	if (c == '\n') {
		linelength[terminal] = 0;
		curr_x[terminal] = 0;
		curr_y[terminal] += 1;
		update_cursor(curr_y[terminal], curr_x[terminal]);
	} else {
		data->character = c;
		if (terminal == 0)
			data->fore = YELLOW;
		else if (terminal == 1)
			data->fore = GREEN;
		else if (terminal == 2)
			data->fore = CYAN;		
		data->back = BLACK;
		curr_x[terminal] += 1;
		linestart[terminal] = curr_x[terminal];
		linelength[terminal] += 1;
		update_cursor(curr_y[terminal], curr_x[terminal]);
	}

	if (curr_y[terminal] >= HEIGHT)
	{
		scroll_line(terminal, 0);
		curr_y[terminal] = HEIGHT - 1;
		update_cursor(curr_y[terminal], curr_x[terminal]);
	}	
}

/*
 * terminal_write
 * DESCRIPTION: writes count bytes from buffer to the appropriate terminal
 * INPUTS: file descriptor, buffer to write, number of bytes to attempt to write
 * OUTPUTS: shows buff on screen
 * SIDE EFFECTS: edits the terminal of process that is writing
 * RETURN VALIE: number of bytes written
 */
int terminal_write(int32_t fd, const void* new_buf, int32_t count)
{
	cli();
	int terminal = get_curr_process_terminal();
	// sti();
	int i;
	int written;
	uint8_t* buf = (uint8_t*)(new_buf);
	if (buf == NULL || count < 0) 
	{
		sti();
		return -1;
	}
	written = 0;
	
	/* terminal write outputs to 128 character buffer in case of keyboard 
	but can **ALSO** write to larger buffers in the case of file reads: limiting a terminal
	write involving a file to 128 char buffers prevents a full file buffer from printing  */

	/* SAFETY TIP: keyboard terminal writes should ALWAYS be passed 128 count since 
					function now accomodates larger than 128 character writes! We don't
					want index out of bounds exceptions when dealing with the keyboard! */
	for (i = 0; i < count; i++)
	{
		// cli();
		if (buf[i] != '\0') {
			put_char_out(buf[i]);
			written++;
		}
		// sti();
	}
	
	linelength[terminal] = 0;
	sti();
	return written;
}

/*
 * terminal_read
 * DESCRIPTION: terminal read device driver
 * INPUTS: reads last 'entered' line from user
 * OUTPUTS: none
 * SIDE EFFECTS: none
 * RETURN VALIE: number of bytes read
 */
int terminal_read(uint32_t* ptr, int offset, int count, uint8_t * buf)
{
	int terminal = get_curr_process_terminal();
	int ret;
	int i;
	if (buf == NULL || count < 0 ) return -1;

	while (1) {
		if (rd[terminal] == 1) break;
	}
	
	ret = 0;
	for (i = 0; i < lastlinelength[terminal] && i < count && lastlinebuffer[terminal][i] != '\0'; i++)
	{
		buf[i] = lastlinebuffer[terminal][i];
		lastlinebuffer[terminal][i] = '\0';
		ret++;
	}
	
	for (i = count; i < lastlinelength[terminal]; i++)
	{
		lastlinebuffer[terminal][i] = '\0';
	}

	cli();
	rd[terminal] = 0;
	lastlinelength[terminal] = 0;
	sti();

	return ret;
}

/*
 * clear_terminal
 * DESCRIPTION: blanks the current terminal
 * INPUTS: none
 * OUTPUTS: none
 * SIDE EFFECTS: clears the screen
 * RETURN VALIE: none
 */
void clear_terminal()
{
	int terminal = get_curr_process_terminal();
	terminal = curr_terminal;
	int srcx;
	int srcy;
	int i;

	cli();
	if (mem == NULL || curr_x[terminal]  < 0 || curr_x[terminal] >= WIDTH || curr_y[terminal] < 0 || curr_y[terminal] >= HEIGHT) return;
//	screen_char_t *t = (screen_char_t*)(mem);
//	screen_char_t *t = (screen_char_t*)(0xb7000);
	screen_char_t *t = (screen_char_t*)(INACTIVE_VID_MEM);

	for (srcx = 0; srcx < WIDTH; srcx++)
	{
		for (srcy = 0; srcy < HEIGHT; srcy++)
		{
			t->character = ASCII_SPACE;
			if (terminal == 0)
				t->fore = YELLOW;
			else if (terminal == 1)
				t->fore = GREEN;
			else if (terminal == 2)
				t->fore = CYAN;
			//t->fore = WHITE;
			t->back = BLACK;
			t++;
		}
	}

	for (i = 0; i < BUFFER_LIMIT; i++)
		linebuffer[terminal][i] = '\0';

	curr_x[terminal] = 0;
	curr_y[terminal] = 0;
	linestart[terminal] = 0;
	lastlinelength[terminal] = 0;
	linelength[terminal] = 0;
	rd[terminal] = 0;
sti();
}

/*
 * terminal_open
 * DESCRIPTION: terminal open device driver
 * INPUTS: none
 * OUTPUTS: none
 * SIDE EFFECTS: initializes the terminal
 * RETURN VALIE: none
 */
int terminal_open()
{
	mem = (unsigned short*)(VID_MEM);
	int j;
    curr_terminal = 0;

	for (j = 0; j < 3; j++)
	{
		curr_x[j] = curr_y[j] = 0;
		blank[j].character = ASCII_SPACE;
		if (j == 0)
			blank[j].fore = YELLOW;
		else if (j == 1)
			blank[j].fore = GREEN;
		else if (j == 2)
			blank[j].fore = CYAN;		
    	//blank.fore = WHITE;
		blank[j].back = BLACK;
		int srcx;
		int srcy;
		int i;
		screen_char_t *t = (screen_char_t*)(mem);
		for (srcx = 0; srcx < WIDTH; srcx++)
		{
			for (srcy = 0; srcy < HEIGHT; srcy++)
			{
				t->character = ASCII_SPACE;
				if (j == 0)
					t->fore = YELLOW;
				else if (j == 1)
					t->fore = GREEN;
				else if (j == 2)
					t->fore = CYAN;
				t->back = BLACK;
				t++;
			}
		}

		for (i = 0; i < BUFFER_LIMIT; i++)
			linebuffer[j][i] = '\0';
			curr_x[j] = 0;
		curr_y[j] = 0;
		linestart[j] = 0;
		lastlinelength[j] = 0;
		linelength[j] = 0;
		rd[j] = 0;
	}
	update_cursor(curr_y[curr_terminal], curr_x[curr_terminal]);
	return 0;
}

/*
 * terminal_close
 * DESCRIPTION: removes terminal variables
 * INPUTS: none
 * OUTPUTS: none
 * SIDE EFFECTS: doesn't do anything really
 * RETURN VALIE: none
 */
int terminal_close()
{
	int terminal = get_curr_process_terminal();
	int i;
	for (i = 0; i < 3; i++)
	{
		linelength[terminal] = lastlinelength[terminal] = linestart[terminal] = rd[terminal] = curr_x[terminal] = curr_y[terminal] = 0;
	}
	return 0;
}

/*
 * update_cursor
 * DESCRIPTION: updates the position of the cursor
 * INPUTS: row,col where cursor should be
 * OUTPUTS: none
 * SIDE EFFECTS: shows cursor on the screen
 * RETURN VALIE: none
 */
void update_cursor(int row, int col)
{
	/* from OSDev */
	unsigned short position = (row * WIDTH) + col;

	outb(CURSOR_LOCATION_LOW, VGA_ADDRESS_REGISTER);
	outb((unsigned char)(position & LOW_BYTE_MASK), VGA_DATA_REGISTER);

	outb(CURSOR_LOCATION_HIGH, VGA_ADDRESS_REGISTER);
	outb((unsigned char)((position >> BYTE_SHIFT) & LOW_BYTE_MASK), VGA_DATA_REGISTER);
}	

/*
 * switch_terminals
 * DESCRIPTION: responds to alt-f1,f2,f3. Chanes terminal based on key combination
 * INPUTS: terminal to switch to
 * OUTPUTS: new terminal's contents
 * SIDE EFFECTS: none
 * RETURN VALIE: none
 */
void switch_terminals(int term)
{
	if (term == curr_terminal) return;
cli();
	uint8_t *from = (uint8_t*)INACTIVE_VID_MEM;
	uint8_t *to = (uint8_t*)(VID_MEM + (curr_terminal+1)*KB4);
	memcpy((void*)to, (const void*)from, KB4);

	to = from;
	from = (uint8_t*)(VID_MEM + (term+1)*KB4);	
	memcpy((void*)to, (const void*)from, KB4);
	swap_vid_mem(curr_terminal, term);
	curr_terminal = term;
sti();
}

void hist_copy_from(char* src){
    int i;
    for(i = 0; i < 128; i++){
    	put_char(BACKSPACE);
    }

    for(i = 0; i < 128; i++){
    	if(src[i] == '\0') break;
    	put_char(src[i]);
    }
	return;
}

/*
 * get_curr_terminal
 * DESCRIPTION: returns the terminla that is being displayed on the screen
 * INPUTS: none
 * OUTPUTS: none
 * SIDE EFFECTS: none
 * RETURN VALIE: curent terminal being displayed
 */
int get_curr_terminal()
{
	return curr_terminal;
}
