#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <idt.h>
#include <timer.h>
#include <gdt.h>
#include <cpuid.h>
#include <kstdio.h>
#include <kstdlib.h>
#include <pci.h>
#include <pic.h>
#include <ata.h>
#include <io.h>
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
extern uint32_t enable_paging(uint32_t*);

PageDirEntry page_directory[1024] __attribute__((aligned(4096)));

// Page Size Bit, if 1, 4MiB, else 4KiB
const uint32_t PS_BIT = (1<<7);

void kernel_main(void) {
    // Initialize page directory
	// Set up Identity Paging
	// frame #00 -> 00000000...000fffff
	// frame #01 -> 00400000...004fffff
	// And so on

	// Map all memory
	kprintf("Using Identity Mapping...\n");
	int from = 0;
	uint32_t four_mib = 1024*1024*4;
	uint32_t* first_pde = page_directory;
	for(int num_entries = 1024; num_entries > 0; num_entries--, from+= four_mib, first_pde++){
		*first_pde = from | 3;	
		*first_pde |= PS_BIT;
	}

	kprintf("Attempting to enter paging mode, page_dir: 0x%x\n", page_directory);
	uint32_t cr3 = enable_paging(page_directory);
	kprintf("In paging mode!, cr3: 0x%x\n", cr3);

	/* Set up GDT */	
	kprintf("Setting up GDT\n");
	setup_gdt();

	/* Init PIT */
	init_PIT_timer();
    
	/* Set up IDT */
	kprintf("Setting up the IDT\n");
	handle_idt_setup();

	load_cpuid();
	print_cpuid_vendor();

	pci_check_all_buses();

	ata_test();

	terminal_proc_start();
}

