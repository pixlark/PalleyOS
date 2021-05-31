#include <stdint.h>
#include <stddef.h>

#ifndef TIO_H
#define TIO_H

typedef enum {
	VGA_COLOR_BLACK = 0,
	VGA_COLOR_BLUE = 1,
	VGA_COLOR_GREEN = 2,
	VGA_COLOR_CYAN = 3,
	VGA_COLOR_RED = 4,
	VGA_COLOR_MAGENTA = 5,
	VGA_COLOR_BROWN = 6,
	VGA_COLOR_LIGHT_GREY = 7,
	VGA_COLOR_DARK_GREY = 8,
	VGA_COLOR_LIGHT_BLUE = 9,
	VGA_COLOR_LIGHT_GREEN = 10,
	VGA_COLOR_LIGHT_CYAN = 11,
	VGA_COLOR_LIGHT_RED = 12,
	VGA_COLOR_LIGHT_MAGENTA = 13,
	VGA_COLOR_LIGHT_BROWN = 14,
	VGA_COLOR_WHITE = 15,
} vga_color;

void inc_cursor();
void term_write_char(char c);
void term_write_char_color(char c, vga_color vc);

void term_write(char* string);
void term_write_color(char* string, vga_color vc);

void term_write_int(int n, unsigned int base);
void term_write_int32(int32_t l, unsigned int base);
void term_write_uint32(uint32_t ul, unsigned int base);

void shift_term_line_up(unsigned int n);


#endif
