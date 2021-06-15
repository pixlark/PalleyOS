#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <idt.h>
#include <tio.h>
#include <timer.h>
#include <gdt.h>
#include <cpuid.h>
#include <keyboard_io.h>
#include <kstdio.h>
#include <memory.h>

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

	/* Set up GDT */	
	term_write("Setting up GDT\n");
	setup_gdt();
    
	/* Set up IDT */
	term_write("Setting up the IDT\n");
	handle_idt_setup();
    
    // Inform the memory unit of our physical memory situation
    load_physical_memory_region_descriptors(multiboot_info);

    // Now, setup paging so we can use our physical memory
    setup_paging();
    
	/* Init Timer and PIT */
//	init_timer();

//	for(int i = 0; i < 256; i++){
//		kprintf("%d\n", i);
//	}
	
	load_cpuid();
	print_cpuid_vendor();

    uint8_t* ptr1 = (uint8_t*) 0x10000000;
	*ptr1 = 0xFF;
	kprintf("0x%x\n", *ptr1);
    //uint8_t* ptr1 = (uint8_t*) (1024 * 1024 + 1);
	/*
    for (uint32_t i = 1024 * 1024; i < 0xffffffff; i += 4) {
        uint32_t* ptr = (uint32_t*) i;
        *ptr = i;
        if (*ptr != i) {
            kprintf("Write error at %x\n", i);
            break;
        }
    }
    kprintf("No write errors!\n");
	*/
    
	//setup_keyboard();
}
