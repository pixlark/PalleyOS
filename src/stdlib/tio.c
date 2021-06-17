/*
 * tio.c gives basic I/O interaction with the terminal
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <kstdio.h>
#include <io.h>
#include <tio.h>

uint16_t* vb = (uint16_t*) 0xB8000;

size_t term_row = 0;
size_t term_col = 0;

// For saving up to 20 pages of terminal
static uint16_t pvb[PVB_SIZE] = {' ' | VGA_COLOR_BLACK << 8};
static int pvb_row = PVB_NUM_ROWS / 2; 

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
	return (uint16_t) uc | (uint16_t) color << 8;
}

static void write_screen_from_pvb();
static void update_cursor(int x, int y);

/* TODO: This should be called whenever tryiing to write
 * at term_row, term_col. The screen could be shifted.
 * If that is the case, this will shift the screen so 
 * the user can see where they are typing
 */
static void fix_screen_pos() {}

static void inc_cursor() {
	term_col++;
	if(term_col >= TERM_WIDTH) {
		term_row++;
		term_col = 0;
	}

	if(term_row >= TERM_HEIGHT-1) tio_shift_term_line(1);
}

void tio_dec_cursor() {
	term_col--;
	if(term_col < 0){
		term_row--;
		term_col = TERM_WIDTH-1;
	}
	if(term_row < 0) term_row = 0;

	int index = term_row*TERM_WIDTH + term_col;
	vb[index] = ' ' | VGA_COLOR_BLACK; 
	pvb[index + pvb_row*TERM_WIDTH] = vb[index];

	update_cursor(term_col+1, term_row);
}

inline void term_write_char(char c) {
	term_write_char_color(c, VGA_COLOR_WHITE);
}

void term_write_char_color(char c, vga_color vc){

	if(c == '\n') {
		term_row ++;
		term_col = 0;
		if(term_row >= TERM_HEIGHT-1)
			tio_shift_term_line(1);
		return;
	}else if(c == '\r'){
		term_col = 0;
		return;
	}

	int index = term_row*TERM_WIDTH + term_col;
	vb[index] = vga_entry(c, vc);
	pvb[index + pvb_row*TERM_WIDTH] = vb[index];
	inc_cursor();
	update_cursor(term_col, term_row);
}

void term_write(char* string) {
   	term_write_color(string, VGA_COLOR_WHITE); 
}
void term_write_color(char* string, vga_color vc) {

	char* c = string;
	while(*c != '\0') {
		term_write_char_color(*c, vc);
		c++;
	}

}

void tio_enable_cursor()
{
	outb(0x3D4, 0x0A);
	outb(0x3D5, (inb(0x3D5) & 0xC0) | 0);

	outb(0x3D4, 0x0B);
	outb(0x3D5, (inb(0x3D5) & 0xE0) | 15);
}

static void update_cursor(int x, int y)
{
	uint16_t pos = y * TERM_WIDTH + x;
	if(pos >= VB_SIZE || pos < 0) return;
 
	outb(0x3D4, 0x0F);
	outb(0x3D5, (uint8_t) (pos & 0xFF));
	outb(0x3D4, 0x0E);
	outb(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
}

static void shift_pvb_to_halfway() {
	
	int shift_dist = pvb_row - (PVB_NUM_ROWS/2);
	if(shift_dist < 0) return; // Shifting down not implemented

	for(int row = 0; row < PVB_NUM_ROWS; row++){
		for(int col = 0; col < TERM_WIDTH; col++){
			int pvb_index = row*TERM_WIDTH + col;
			int prev_pvb_index = (row + shift_dist)*TERM_WIDTH + col;
			if(row + shift_dist > PVB_NUM_ROWS)
				pvb[pvb_index] = 0; 
			else
				pvb[pvb_index] = pvb[prev_pvb_index];
		}
	}

	pvb_row = PVB_NUM_ROWS/2;
}

void tio_shift_term_line(int n) {
	int desired_pvb_row = pvb_row + n;

	// If we hit top, no more scrolling, just print the screen
	if(pvb_row + n < 0) {
		pvb_row = 0;
	}
	else if(pvb_row > PVB_NUM_ROWS-1){
		shift_pvb_to_halfway();
		term_row = 0;
	}
	else{
		pvb_row = desired_pvb_row;
		term_row -= n;
	}

	write_screen_from_pvb();
}

static void write_screen_from_pvb(){
	disable_interrupts();
	int pvb_offset = pvb_row*TERM_WIDTH;
	for(int i = 0; i < VB_SIZE; i++)
	   	vb[i] = pvb[pvb_offset+i];
	enable_interrupts();
}

