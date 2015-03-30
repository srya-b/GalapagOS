#include "terminal.h"

#define VID_MEM 0xb8000
#define WIDTH 80
#define HEIGHT 25
#define INDEX(x,y) (x + (WIDTH * y))
#define BLACK 0
#define WHITE 15
#define ASCII_SPACE 12
#define BACKSPACE 0x08
#define TAB 0x09
#define DELETE 0x7f
#define NEW_LINE 0xa
#define BUFFER_LIMIT 128

static int x;
static int y;
static unsigned short *mem;
static screen_char_t blank;
static int line_length;
static int last_line_length;
static int line_start;
static uint8_t line_buffer[BUFFER_LIMIT+1];
static uint8_t last_line_buffer[BUFFER_LIMIT+1];
static int read;


/***************************************/
/* LOCK FOR READ AND WRITE!!!!!!!!!!!! */
/***************************************/
void scroll_line()
{
	unsigned short int character;
	screen_char_t *ptr = (screen_char_t*)(&character);
	ptr->character = ' ';
	ptr->fore = BLACK;
	ptr->back = BLACK;

	int i = y - HEIGHT + 1;
	memcpy(mem, mem + i*WIDTH, (HEIGHT - i) * WIDTH * 2);
	memset_word(mem + (HEIGHT - i)*WIDTH, character, WIDTH);
	y = HEIGHT - 1;	
}

void clear_buffer()
{
	int i;
	for (i = 0; i < BUFFER_LIMIT; i++)
		line_buffer[i] = '\0';
}

void put_char(char c)
{
	screen_char_t *data = (screen_char_t*)(mem + INDEX(x,y));

	if (line_length < BUFFER_LIMIT)
	{
		if (c == '\t') 
		{
			int i;
		 	x += 8;
		 	for (i = 0; i < 8; i++)
		 	{
		 		line_buffer[line_length] = ' ';
		 		line_length++;
		 	}
		 	update_cursor(y, x);
		}		 	
		if (x >= WIDTH)
		{
			x = 0;
			y++;
			update_cursor(y, x);
		}
		if (c >= ' ') {
			line_buffer[line_length] = c;
			data->character = c;
			data->fore = WHITE;
			data->back = BLACK;
			update_cursor(y, x);
			x++;
			line_length++;
		} 
	}
		if (c == BACKSPACE) {
			if (line_length > 0) x--;
			data = (screen_char_t*)(mem + INDEX(x,y));
			*data = blank;
			if (line_length > 0) line_length--;
			line_buffer[line_length] = '\0';
			update_cursor(y, x);
		}
	
	if (c == '\n' ) {
		last_line_length = line_length;
		line_length = 0;
		line_start = 0;
		int i;
		for (i = 0; i < last_line_length; i++)
		{
			last_line_buffer[i] = line_buffer[i];
			line_buffer[i] = '\0';
		}
		x = 0;
		y++;
		read = 1;
		update_cursor(y, x);
	}	

	if (y >= HEIGHT)
	{
		scroll_line();
		y = HEIGHT - 1;
		update_cursor(y, x);
	}
}

void put_char_out(char c)
{
	screen_char_t *data = (screen_char_t*)(mem + INDEX(x,y));

	if (x >= WIDTH)
	{
		line_start = 0;
		x = 0;
		y++;
		update_cursor(y, x);
	}

	if (c == '\n' ) {
		line_length = 0;
		x = 0;
		y++;
		update_cursor(y, x);

	} else {
		data->character = c;
		data->fore = WHITE;
		data->back = BLACK;
		x++;
		line_start = x;
		line_length++;
		update_cursor(y, x);

	}

	if (y >= HEIGHT)
	{
		scroll_line();
		y = HEIGHT - 1;
		update_cursor(y, x);

	}	
}

int terminal_write(int count, uint8_t *buf)
{
	int i;
	int written;
	if (buf == NULL || count < 0) return -1;

	written = 0;
	
	/* terminal write outputs to 128 character buffer in case of keyboard 
	but can **ALSO** write to larger buffers in the case of file reads: limiting a terminal
	write involving a file to 128 char buffers prevents a full file buffer from printing  */

	/* SAFETY TIP: keyboard terminal writes should ALWAYS be passed 128 count since 
					function now accomodates larger than 128 character writes! We don't
					want index out of bounds exceptions when dealing with the keyboard! */
	for (i = 0; i < count; i++)
	{
		put_char_out(buf[i]);
		written++;
	}
	
	line_length = 0;
	return written;
}

int terminal_read(int count, uint8_t *buf)
{
	int ret;
	int i;
	if (buf == NULL || count < 0 || count > 128) return -1;

	while (read == 0) { }
	
	ret = 0;
	for (i = 0; i < last_line_length && i < count && last_line_buffer[i] != '\0'; i++)
	{
		buf[i] = last_line_buffer[i];
		last_line_buffer[i] = '\0';
		ret++;
	}
	
	for (i = count; i < last_line_length; i++)
	{
		last_line_buffer[i] = '\0';
	}

	read = 0;
	last_line_length = 0;
	
	return ret;
}

void clear_terminal()
{
	int srcx;
	int srcy;
	int i;

	if (mem == NULL || x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return;
	screen_char_t *t = (screen_char_t*)(mem);

	for (srcx = 0; srcx < WIDTH; srcx++)
	{
		for (srcy = 0; srcy < HEIGHT; srcy++)
		{
			t->character = ASCII_SPACE;
			t->fore = BLACK;
			t->back = BLACK;
			t++;
		}
	}

	for (i = 0; i < BUFFER_LIMIT; i++)
		line_buffer[i]  = '\0';

	x = 0;
	y = 0;
	line_start = 0;
	last_line_length = 0;
	line_length = 0;
	read = 0;

	update_cursor(y, x);

}

int terminal_open()
{
	mem = (unsigned short*)(VID_MEM);
	x = y = 0;
	blank.character = ASCII_SPACE;
	blank.fore = BLACK;
	blank.back = BLACK;

	clear_terminal();

	update_cursor(y, x);

	return 0;
}
void update_cursor(int row, int col)
{
	/* from OSDev */
	unsigned short position=(row*80)+col;

	outb(0x0F,0x3D4);
	outb((unsigned char)(position&0xFF),0x3D5);

	outb(0x0E,0x3D4);
	outb((unsigned char)((position>>8)&0xFF), 0x3D5);
}	
