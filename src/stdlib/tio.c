/*
 * tio.c gives basic I/O interaction with the terminal
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <tio.h>

uint16_t* video_buff = (uint16_t*) 0xB8000;

size_t term_row = 0;
size_t term_col = 0;
static const size_t TERM_WIDTH = 80; 
static const size_t TERM_HEIGHT = 25; 

static inline uint16_t vga_entry(unsigned char uc, uint8_t color);

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
	return (uint16_t) uc | (uint16_t) color << 8;
}

void inc_cursor() {
	term_col++;
	if(term_col >= TERM_WIDTH) {
		term_row++;
		term_col = 0;
	}

	if(term_row >= TERM_HEIGHT-1) shift_term_line_up(1);
}

inline void term_write_char(char c) {
	term_write_char_color(c, VGA_COLOR_WHITE);
}

void term_write_char_color(char c, vga_color vc){

	if(c == '\n') {
		term_row ++;
		term_col = 0;
		if(term_row > TERM_HEIGHT)
			shift_term_line_up(1);
		return;
	}else if(c == '\r'){
		term_col = 0;
		return;
	}

	int index = term_row*TERM_WIDTH + term_col;
	video_buff[index] = vga_entry(c, vc);
	inc_cursor();
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

void term_write_int(int n, unsigned int base) {
	term_write_int32((int32_t) n, base);
}

void term_write_int32(int32_t n, unsigned int base) {

	bool negative = n < 0;	
	if(base == 16) term_write("0x");
	else if(base == 2) term_write("0b");
	else if(base == 8) term_write("0");

	if(negative)
		n = ~n + 1; // convert to positive number 

	// Calculate length
	uint8_t len = 0;
	int cpy = n;
	while(cpy > 0){
		len ++;	
		cpy /= base;
	}	

	if(len == 0) len++;
	
	if(negative) len += 1;
		
	char buf[len];
	for(int i = 0; i < len; i++)
		buf[i] = '0';

	int i = 0;
	while(n > 0){
		if(n % base <= 10) 
			buf[i] = (n % base) + '0';
		else
			buf[i] = ((n-10) % base) + 'a';
		n /= base;	
		i++;
	}

	if(negative)
		buf[len-1] = '-';

	for(int i = len-1; i >= 0; i--){
		term_write_char(buf[i]);
	}
}


void term_write_uint32(uint32_t n, unsigned int base) {
	
	if(base == 16) term_write("0x");
	else if(base == 2) term_write("0b");
	else if(base == 8) term_write("0");

	// Calculate length
	uint8_t len = 0;
	int cpy = n;
	while(cpy > 0){
		len ++;	
		cpy /= base;
	}	

	if(base == 16)
		len = 8;	
		
	char buf[len];
	for(int i = 0; i < len; i++)
		buf[i] = '0';

	int i = 0;
	while(n > 0){
		if(n % base <= 9) 
			buf[i] = (n % base) + '0';
		else
			buf[i] = ((n-10) % base) + 'a';
		n /= base;	
		i++;
	}

	for(int i = len-1; i >= 0; i--){
		term_write_char(buf[i]);
	}
}

// TODO: Make this save the old lines in memory so we can access late (scroll down)
void shift_term_line_up(unsigned int n) {
	for(uint16_t i = 0; i < TERM_HEIGHT - n; i++) {
		for(uint16_t j = 0; j < TERM_WIDTH; j++) {
			video_buff[i*TERM_WIDTH + j] = video_buff[(i+n)*TERM_WIDTH + j];
		}
	}
	term_row -= n;

	/*
	for(uint16_t i = TERM_HEIGHT - n; i < TERM_HEIGHT; i++) {
		for(uint16_t j = 0; j < TERM_WIDTH; j++) {
			video_buff[i*TERM_WIDTH + j] = vga_entry(' ', VGA_COLOR_BLACK);	
		}
	}
	*/
}

