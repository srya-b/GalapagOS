#ifndef _TERMINAL_H
#define _TERMINAL_H

#include "lib.h"
#include "types.h"

typedef struct screen_char {
	uint8_t character : 8;
	uint8_t fore : 4;
	uint8_t back : 4;
} __attribute__((packed)) screen_char_t;

void scroll_line();
void put_char(char c);
void put_char_out(char c);
int terminal_open();
void clear_terminal();
int terminal_write(int count, uint8_t *buf);
int terminal_read(int count, uint8_t *buf);
void update_cursor(int row, int col);
#endif
