#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <idt.h>
#include <timer.h>
#include <gdt.h>
#include <cpuid.h>
#include <kstdio.h>
#include <pci.h>
#include <terminal_proc.h>

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

	// Set up Identity Paging
	// frame #00 -> 00000000...000fffff
	// frame #01 -> 00400000...004fffff
	// And so on

	// Map all memory
	kprintf("Using Identity Mapping...\n");
	int from = 0;
	uint32_t four_mib = 1024*1024*4;
	uint32_t* first_pde = page_directory;
	for(int num_entries = 256; num_entries > 0; num_entries--, from+= four_mib, first_pde++){
		*first_pde = from | 3;	
		*first_pde |= PS_BIT;
	}

	kprintf("Attempting to enter paging mode\n");
	enable_paging(page_directory);
	kprintf("In paging mode!\n");

	/* Set up GDT */	
	kprintf("Setting up GDT\n");
	setup_gdt();
    
	/* Set up IDT */
	kprintf("Setting up the IDT\n");
	handle_idt_setup();

    // 3735928559
    
	/* Init Timer and PIT */
//	init_timer();

	load_cpuid();
	print_cpuid_vendor();

//	pci_check_all_buses();

	terminal_proc_start();

}

