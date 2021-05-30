#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "paging.h"

#if defined(__linux__)
#error "You are not using the cross compiler, idiot"
#endif

#if !defined(__i386__)
#error "must use x86"
#endif

//
// Text output
//

/* Hardware text mode color constants. */
enum vga_color {
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
};


static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
	return (uint16_t) uc | (uint16_t) color << 8;
}

//
// Paging
//
extern void flush_tlb();
PageDirEntry page_directory[1024];

void kernel_main(void) {
    // Initialize page directory
    //for (int i = 0; i < 1024; i++) {
    //    PageDirEntry entry = 0;
    //    entry |= 0b001000111;
    //    entry |= (1 << 31);
    //    page_directory[i] = entry;
    //}

    // Map 0x100000 -> 0x000000
    //     0x1B8000 -> 0x0B8000
    //page_directory[1] &= ~(1 << 31);
    
    //flush_tlb();
    
	uint16_t* t_buff = (uint16_t*) 0xB8000;
	for (size_t y = 0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t index = y * VGA_WIDTH + x;
			t_buff[index] = vga_entry(' ', VGA_COLOR_BLACK);
		}
	}

	t_buff[0] = vga_entry('H', VGA_COLOR_WHITE);
	t_buff[1] = vga_entry('I', VGA_COLOR_WHITE);
	t_buff[2] = vga_entry('!', VGA_COLOR_WHITE);
}

