#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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

void kernel_main(MultibootInfo* multiboot_info, uint32_t magic) {
    // Get RAM info from GRUB
    if (magic != 0x2BADB002) {
        return;
    }

	/* Set up IDT */
	kprintf("Setting up the IDT\n");
	handle_idt_setup();

	/* Set up GDT */	
	kprintf("Setting up GDT\n");
	setup_gdt();

	/* Init PIT */
	init_PIT_timer();
    
    
    // Inform the memory unit of our physical memory situation
    load_physical_memory_region_descriptors(multiboot_info);

    // Now, setup paging so we can use our physical memory
    setup_paging();

    initialize_kheap();

	/* Init Timer and PIT */
//	init_timer();

	load_cpuid();
	print_cpuid_vendor();

//	pci_check_all_buses();

    /*
    kprintf("BEFORE ANYTHING\n");
    kheap_dump();
    uint8_t* my_memory = kheap_alloc(100);
    kprintf("ALLOCATED 100 BYTES\n");
    kheap_dump();
    uint8_t* more_memory = kheap_alloc(1000);
    kprintf("ALLOCATED 1000 BYTES\n");
    kheap_dump();
    kheap_free(my_memory);
    kprintf("FREED 100 BYTES\n");
    kheap_dump();
    uint8_t* even_more_memory = kheap_alloc(15);
    kprintf("ALLOCATED 15 BYTES\n");
    kheap_dump();
    kheap_free(even_more_memory);
    kprintf("FREED 15 BYTES\n");
    kheap_dump();*/

	terminal_proc_start();
}
