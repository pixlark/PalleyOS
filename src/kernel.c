#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <idt.h>
#include <tio.h>
#include <gdt.h>

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
extern void load_page_dir(unsigned int*);
extern void enable_paging(uint32_t*);

PageDirEntry page_directory[1024] __attribute__((aligned(4096)));

// Page Size Bit, if 1, 4MiB, else 4KiB
const uint32_t PS_BIT = (1<<7);

void kernel_main(void) {
    // Initialize page directory
    for (int i = 0; i < 1024; i++) {
        PageDirEntry entry = 0;
        entry |= 0b000000011;
		entry |= PS_BIT;
        page_directory[i] = entry;
    }

	term_write("Page Directory addr: ");
	term_write_uint32(page_directory, 16);
	term_write("\n");

	// Set up Identity Paging
	// frame #00 -> 00000000...000fffff
	// frame #01 -> 00400000...004fffff
	// And so on

	//Try mapping the first 16 MiB
	term_write("Using Identity Mapping...\n");
	int from = 0;
	uint32_t four_mib = 1024*1024*4;
	uint32_t* first_pde = page_directory;
	for(int num_entries = 256; num_entries > 0; num_entries--, from+= four_mib, first_pde++){
		*first_pde = from | 3;	
		*first_pde |= PS_BIT;
	}

	term_write("Attempting to enter paging mode\n");
	enable_paging(page_directory);
	term_write("In paging mode!\n");

	/* Set up GDT */	
	term_write("Setting up GDT\n");
	setup_gdt();

	/* Set up IDT */
	term_write("Setting up the IDT\n");
	handle_idt_setup();

	/* Test Interrupt */
}

