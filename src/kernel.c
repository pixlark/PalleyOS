#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <tio.h>

#include "paging.h"

#if defined(__linux__)
#error "You are not using the cross compiler, silly goose"
#endif

#if !defined(__i386__)
#error "must use x86"
#endif


//
// Paging
//
extern void flush_tlb();
PageDirEntry page_directory[1024];

void kernel_main(void) {
    // Initialize page directory
    for (int i = 0; i < 1024; i++) {
        PageDirEntry entry = 0;
        entry |= 0b001000111;
        entry |= (1 << 31);
        page_directory[i] = entry;
    }

    // Map 0x100000 -> 0x000000
    //     0x1B8000 -> 0x0B8000
    page_directory[1] &= ~(1 << 31);
    
    //flush_tlb();
    
	term_write_color("HOWDY THERE PARTNER, pd: ", VGA_COLOR_WHITE);	
	term_write_int(page_directory, 16);
	term_write("\npd[0]: ");
	term_write_uint32(page_directory[0], 16);
	term_write("\npd[1]: ");
	term_write_uint32(page_directory[1], 16);

}

