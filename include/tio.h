#ifndef TIO_H
#define TIO_H

#include <stdint.h>
#include <stddef.h>

#define TERM_WIDTH 80
#define TERM_HEIGHT 25
#define VB_SIZE TERM_WIDTH * TERM_HEIGHT

#define PVB_PAGES 10
#define PV_PAGE_SIZE TERM_WIDTH * TERM_HEIGHT
#define PVB_SIZE VB_SIZE * PVB_PAGES
#define PVB_NUM_ROWS PVB_PAGES * TERM_HEIGHT


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

void tioEnableCursor();

void tioWriteChar(char c);
void tioWriteCharColor(char c, vga_color vc);

void tioWrite(char* string);
void tioWriteColor(char* string, vga_color vc);

void termWriteInt(int n, unsigned int base);
void termWriteInt32(int32_t l, unsigned int base);
void tioWrite_uint32(uint32_t ul, unsigned int base);

void tioShiftTermLineProtected(int n);

void tioIncCursor();
void tioDecCursor();
void tioBackspace();

void tioShiftRight();
void tioShiftLeft();

#endif
