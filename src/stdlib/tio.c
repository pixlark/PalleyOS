/*
 * tio.c gives basic I/O interaction with the terminal
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <kstdio.h>
#include <io.h>
#include <tio.h>

static uint16_t* vb = (uint16_t*) 0xB8000;

static int term_row = 0;
static int term_col = 0;

static uint16_t pvb[PVB_SIZE] = {' ' | VGA_COLOR_BLACK << 8};
static int pvb_row = PVB_NUM_ROWS / 2; 

static void writeScreenFromPVB();
static void updateCursor(int x, int y);

static inline uint16_t vgaEntry(unsigned char uc, uint8_t color) {
	return (uint16_t) uc | (uint16_t) color << 8;
}

static void updateCursor(int x, int y);
static void tioShiftTermLine(int n);

/* TODO: This should be called whenever tryiing to write
 * at term_row, term_col. The screen could be shifted.
 * If that is the case, this will shift the screen so 
 * the user can see where they are typing
 */
void tioIncCursor() {
	term_col++;
	if(term_col >= TERM_WIDTH) {
		term_row++;
		term_col = 0;
	}

	if(term_row >= TERM_HEIGHT-1) {
        tioShiftTermLine(1);
        writeScreenFromPVB();
    }
	updateCursor(term_col, term_row);
}

void tioDecCursor() {
	term_col--;
	if(term_col < 0){
		term_row--;
		term_col = TERM_WIDTH-1;
	}
	if(term_row < 0) term_row = 0;

	updateCursor(term_col, term_row);
}

void tioBackspace() {
    tioDecCursor();

	int index = term_row*TERM_WIDTH + term_col;
	pvb[index + pvb_row*TERM_WIDTH] = vb[index];
}

// Shfits all characters visible to the right from the cursor and replaces
// it with a blank
void tioShiftRight() {
    for(int i = VB_SIZE-1; i > (term_row*TERM_WIDTH + term_col); i--) 
       vb[i] = vb[i-1]; 
	for(int j = pvb_row*TERM_WIDTH+VB_SIZE-1; j > (pvb_row*TERM_WIDTH + term_row*TERM_WIDTH) + term_col; j--)
		pvb[j] = pvb[j-1];
    vb[(term_row*TERM_WIDTH) + term_col] = ' ' | VGA_COLOR_BLACK << 8;
    pvb[(pvb_row*TERM_WIDTH) + (term_row*TERM_WIDTH) + term_col] = ' ' | VGA_COLOR_BLACK << 8;
}

// Shfits all characters visible to the left from the cursor and replaces
// it with a blank
void tioShiftLeft() {
    size_t vb_index = (term_row*TERM_WIDTH + term_col) - 1;
    if(term_col == 0) vb_index += 1;
    for(size_t i = vb_index; i < VB_SIZE; i++) 
       vb[i] = vb[i+1]; 

	size_t pvb_index = (pvb_row*TERM_WIDTH + +term_row*TERM_WIDTH + term_col) - 1;
    if(term_col == 0) pvb_index += 1;
	for(size_t j = pvb_index; j < (size_t)(pvb_row*TERM_WIDTH + VB_SIZE); j++)
		pvb[j] = pvb[j+1];
	writeScreenFromPVB();
}

inline void tioWriteChar(char c) {
	tioWriteCharColor(c, VGA_COLOR_WHITE);
}

void tioWriteCharColor(char c, vga_color vc){

	if(c == '\n') {
		term_row ++;
		term_col = 0;
		if(term_row >= TERM_HEIGHT-1){
			tioShiftTermLine(1);
            writeScreenFromPVB();
        }
		return;
	}else if(c == '\r'){
		term_col = 0;
		return;
	}

	int index = term_row*TERM_WIDTH + term_col;
	vb[index] = vgaEntry(c, vc);
	pvb[index + pvb_row*TERM_WIDTH] = vb[index];
	tioIncCursor();
	updateCursor(term_col, term_row);
}

void tioWrite(char* string) {
   	tioWriteColor(string, VGA_COLOR_WHITE); 
}
void tioWriteColor(char* string, vga_color vc) {

	char* c = string;
	while(*c != '\0') {
		tioWriteCharColor(*c, vc);
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

static void updateCursor(int x, int y)
{
	int16_t pos = y * TERM_WIDTH + x;
	if(pos >= VB_SIZE) return;
 
	outb(0x3D4, 0x0F);
	outb(0x3D5, (uint8_t) (pos & 0xFF));
	outb(0x3D4, 0x0E);
	outb(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
}

static void shiftPVBToHalfway() {
	
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

void tioShiftTermLine(int n) {
	int desired_pvb_row = pvb_row + n;

	// If we hit top, no more scrolling, just print the screen
	if(desired_pvb_row < 0) {
		pvb_row = 0;
	}
	else if(pvb_row > PVB_NUM_ROWS-TERM_HEIGHT-1){
		shiftPVBToHalfway(); 
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

    tioShiftTermLine(n);
	writeScreenFromPVB();
}

static void writeScreenFromPVB(){
	cli();
	int pvb_offset = pvb_row*TERM_WIDTH;
	for(int i = 0; i < VB_SIZE; i++)
	   	vb[i] = pvb[pvb_offset+i];
	sti();
}

