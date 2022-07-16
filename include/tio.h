#ifndef TIO_H
#define TIO_H

#include <stdint.h>
#include <stddef.h>

#define TERM_WIDTH 80
#define TERM_HEIGHT 24
#define VB_SIZE TERM_WIDTH * TERM_HEIGHT

#define FVB_PAGES 10
#define PV_PAGE_SIZE TERM_WIDTH * TERM_HEIGHT
#define PVB_SIZE VB_SIZE * FVB_PAGES
#define PVB_NUM_ROWS FVB_PAGES * TERM_HEIGHT

typedef enum {
    SHIFT_DIRECTION_UP,
    SHIFT_DIRECTION_DOWN
} Shift_Direction;

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
} Vga_Color;

typedef uint16_t Vga_Entry;

void tio_enable_cursor();

void tio_dec_cursor();
void tio_backspace();
void tio_init();

void tio_write(char* string);
void tio_write_color(char* string, Vga_Color vc);

void tio_cursor_inc();
void tio_cursor_dec();
bool tio_try_cursor_inc(int chars_typed, int padding_front);
bool tio_try_cursor_dec(int padding_size);

void tio_shift_right();
void tio_shift_left();

void tio_shift_view(Shift_Direction dir, uint32_t amt);

#endif
