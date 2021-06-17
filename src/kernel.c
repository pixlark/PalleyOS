#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <idt.h>
#include <tio.h>
#include <io.h>
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

	/* Set up IDT */
	kprintf("Setting up the IDT\n");
	handle_idt_setup();

	/* Set up GDT */	
	kprintf("Setting up GDT\n");
	setup_gdt();
    
    
    // Inform the memory unit of our physical memory situation
    load_physical_memory_region_descriptors(multiboot_info);

    // Now, setup paging so we can use our physical memory
    setup_paging();

	/* Init Timer and PIT */
//	init_timer();

//	for(int i = 0; i < 256; i++){
//		kprintf("%d\n", i);
//	}
	
	//load_cpuid();
	//print_cpuid_vendor();

    uint8_t* ptr = 0x400000;
    *ptr = 0xff;
    kprintf("(0x400000): %x\n", *ptr);

    ptr = 0x7fffff;
    *ptr = 0xff;
    kprintf("(0x7fffff): %x\n", *ptr);

    ptr = 0x800000;
    *ptr = 0xff;
    kprintf("(0x800000): %x\n", *ptr);

    ptr = 0xffffffff;
    *ptr = 0xff;
    kprintf("(0xffffffff): %x\n", *ptr);

    ptr = 0x8000;
    *ptr = 0xff;
    kprintf("(0x8000): %x\n");
    
	//setup_keyboard();
}
