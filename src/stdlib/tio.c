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

int term_row = 0;
int term_col = 0;

static uint16_t pvb[PVB_SIZE] = {' ' | VGA_COLOR_BLACK << 8};
static int pvb_row = PVB_NUM_ROWS / 2; 

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
	return (uint16_t) uc | (uint16_t) color << 8;
}

static void write_screen_from_pvb();
static void update_cursor(int x, int y);
static void tio_shift_term_line(int n);

/* TODO: This should be called whenever tryiing to write
 * at term_row, term_col. The screen could be shifted.
 * If that is the case, this will shift the screen so 
 * the user can see where they are typing
 */
static void fix_screen_pos() {
    if(term_row < TERM_HEIGHT && term_row > 0) return;
    

    write_screen_from_pvb();
}

void tioIncCursor() {
	term_col++;
	if(term_col >= TERM_WIDTH) {
		term_row++;
		term_col = 0;
	}

	if(term_row >= TERM_HEIGHT-1) {
        tio_shift_term_line(1);
        write_screen_from_pvb();
    }
	update_cursor(term_col, term_row);
}

void tioDecCursor() {
	fix_screen_pos();
	term_col--;
	if(term_col < 0){
		term_row--;
		term_col = TERM_WIDTH-1;
	}
	if(term_row < 0) term_row = 0;

	update_cursor(term_col, term_row);
}

void tio_backspace() {
    tioDecCursor();

	int index = term_row*TERM_WIDTH + term_col;
	pvb[index + pvb_row*TERM_WIDTH] = vb[index];
}

void tio_shift_right() {
    for(size_t i = VB_SIZE-1; i > (term_row*TERM_WIDTH + term_col); i--) 
       vb[i] = vb[i-1]; 
	for(size_t j = pvb_row*TERM_WIDTH+VB_SIZE-1; j > (pvb_row*TERM_WIDTH + term_row*TERM_WIDTH) + term_col; j--)
		pvb[j] = pvb[j-1];
    vb[(term_row*TERM_WIDTH) + term_col] = ' ' | VGA_COLOR_BLACK << 8;
    pvb[(pvb_row*TERM_WIDTH) + (term_row*TERM_WIDTH) + term_col] = ' ' | VGA_COLOR_BLACK << 8;
}

void tio_shift_left() {
    size_t vb_index = (term_row*TERM_WIDTH + term_col) - 1;
    if(term_col == 0) vb_index += 1;
    for(size_t i = vb_index; i < VB_SIZE; i++) 
       vb[i] = vb[i+1]; 

	size_t pvb_index = (pvb_row*TERM_WIDTH + +term_row*TERM_WIDTH + term_col) - 1;
    if(term_col == 0) pvb_index += 1;
	for(size_t j = pvb_index; j < (size_t)(pvb_row*TERM_WIDTH + VB_SIZE); j++)
		pvb[j] = pvb[j+1];
	write_screen_from_pvb();
}

inline void term_write_char(char c) {
	term_write_char_color(c, VGA_COLOR_WHITE);
}

void term_write_char_color(char c, vga_color vc){

	if(c == '\n') {
		term_row ++;
		term_col = 0;
		if(term_row >= TERM_HEIGHT-1){
			tio_shift_term_line(1);
            write_screen_from_pvb();
        }
		return;
	}else if(c == '\r'){
		term_col = 0;
		return;
	}

	int index = term_row*TERM_WIDTH + term_col;
	vb[index] = vga_entry(c, vc);
	pvb[index + pvb_row*TERM_WIDTH] = vb[index];
	tioIncCursor();
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

void tioEnableCursor()
{
	outb(0x3D4, 0x0A);
	outb(0x3D5, (inb(0x3D5) & 0xC0) | 0);

	outb(0x3D4, 0x0B);
	outb(0x3D5, (inb(0x3D5) & 0xE0) | 15);
}

static void update_cursor(int x, int y)
{
	int16_t pos = y * TERM_WIDTH + x;
	if(pos >= VB_SIZE) return;
 
	outb(0x3D4, 0x0F);
	outb(0x3D5, (uint8_t) (pos & 0xFF));
	outb(0x3D4, 0x0E);
	outb(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
}

static void shift_pvb_to_halfway() {
	
    int shift_dist = PVB_NUM_ROWS/4;

	for(uint32_t row = 0; row < PVB_NUM_ROWS; row++){
		for(uint32_t col = 0; col < TERM_WIDTH; col++){
			uint32_t pvb_index = row*TERM_WIDTH + col;
			uint32_t prev_pvb_index = (row + shift_dist)*TERM_WIDTH + col;
			if(row + shift_dist > PVB_NUM_ROWS)
				pvb[pvb_index] = ' ' | VGA_COLOR_BLACK;
			else
				pvb[pvb_index] = pvb[prev_pvb_index];
		}
	}

	pvb_row -= shift_dist;
}

void tio_shift_term_line(int n) {
	int desired_pvb_row = pvb_row + n;

	// If we hit top, no more scrolling, just print the screen
	if(desired_pvb_row < 0) {
		pvb_row = 0;
	}
	else if(pvb_row > PVB_NUM_ROWS-TERM_HEIGHT-1){
		shift_pvb_to_halfway(); 
	}
	else{
		pvb_row = desired_pvb_row;
		term_row -= n;
	}
}

void tioShiftTermLineProtected(int n) {
	if(term_row - n < 0){
        return;
    }

    tio_shift_term_line(n);
	write_screen_from_pvb();
}

static void write_screen_from_pvb(){
	cli();
	int pvb_offset = pvb_row*TERM_WIDTH;
	for(int i = 0; i < VB_SIZE; i++)
	   	vb[i] = pvb[pvb_offset+i];
	sti();
}

