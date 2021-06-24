#include "test-lib.h"

int kernelMain() {
    // TODO: This stuff which exists both in this kernelMain and in
    // the normal kernelMain should be abstracted out into some kind
    // of 'setup' function
    
    // Get RAM info from GRUB
    if (magic != 0x2BADB002) {
        return;
    }

	/* Set up IDT */
	kprintf("Setting up the IDT\n");
	idtHandleSetup();

	/* Set up GDT */	
	kprintf("Setting up GDT\n");
	gdtInit();

	/* Init PIT */
	initPITTimer();
    
    
    // Inform the memory unit of our physical memory situation
    loadPhysicalMemoryRegionDescriptors(multiboot_info);

    // Now, setup paging so we can use our physical memory
    setupPaging();

    // TODO: Put some actual tests here!
}
