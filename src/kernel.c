#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <idt.h>
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
extern void load_page_dir(unsigned int*);
extern void enable_paging(uint32_t*);

PageDirEntry page_directory[1024] __attribute__((aligned(4096)));

// Page Size Bit, if 1, 4MiB, else 4KiB
const uint32_t PS_BIT = (1<<7);


// Theoretical Mapping from Virtual to Physical Address
intptr_t vtp(uint32_t virtualaddr) {
	term_write("Virtual Addr: ");
	term_write_uint32(virtualaddr, 16);
	uint32_t offset = virtualaddr & (0x3fffff); // Last 22 bits
	uint32_t pde_index = virtualaddr & (0xffc << 20);
	pde_index >>= 20;

	term_write(", index: ");
	term_write_uint32(pde_index, 16);

	term_write(", pde: ");
	term_write_uint32(page_directory[pde_index], 16);
	term_write(" ");

	// 39:32 of final, 20:13 of pde
	uint32_t upper = page_directory[pde_index] & (0xf << 13);

	term_write("upper: ");
	term_write_uint32(upper, 16);
	term_write(" ");

	// 31:22 of final, 31:22 of pde
	uint32_t lower = page_directory[pde_index] & (0xffc << 20);

	term_write("\nlower: ");
	term_write_uint32(lower, 16);
	term_write(" ");

	intptr_t physaddrcpy = upper << 15 | lower | offset;
	term_write("PhysAddr: ");
	term_write_uint32(physaddrcpy, 16);
	term_write("\n");

	return physaddrcpy;
}

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
	
	term_write("Setting up the IDT\n");
	handle_idt_setup();
}

