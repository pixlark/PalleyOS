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
#include <ide.h>
#include <io.h>
#include <kshell.h>
#include <memory.h>
#include <idt.h>
#include <kheap.h>
#include <timer.h>

#if defined(__linux__)
#error "You are not using the cross compiler, silly goose"
#endif

#if !defined(__i386__)
#error "must use x86"
#endif

void testRealloc(int change) {
    void* ptr = kheapAlloc(100);
    kprintf("  = BEFORE =\n");
    kheapDump();
    ptr = kheapRealloc(ptr, 100 + change);
    kprintf("  = AFTER =\n");
    kheapDump();
    kheapFree(ptr);
}

void kernelMain(MultibootInfo* multiboot_info, uint32_t magic) {
    // Get RAM info from GRUB
    if (magic != 0x2BADB002) {
        return;
    }

	/* Set up IDT */
	kprintf("Setting up the IDT\n");
	idtInit();

	/* Set up GDT */	
	kprintf("Setting up GDT\n");
	gdtInit();

	/* Init Timer and PIT */
    kprintf("Initializing PIT Timer\n");
	initPITTimer();
    
    // Inform the memory unit of our physical memory situation
    loadPhysicalMemoryRegionDescriptors(multiboot_info);

    // Now, setup paging so we can use our physical memory
    setupPaging();

    kheapInit();
    
    {
        // Test heap
        kprintf("=== SAME SIZE ALLOCATION ===\n");
        testRealloc(0);
        kprintf("=== INCREASING ALLOCATION ===\n");
        testRealloc(100);
        kprintf("=== DECREASING ALLOCATION ===\n");
        testRealloc(-50);

        kprintf("=== ALIGNED ALLOCATION ===\n");
        kheapAlignedAlloc(100, 512);
        kheapDump();
    }

	loadCpuid();
	cpuidPrintVendor();

	pciCheckAllBuses();

    kprintf("Starting Timer\n");

    PITResult counter = pitAddCounter();
    uint8_t counter_id = counter.counter_id;
    if(counter.isError == true){
        if(counter.error == PITCountersFull)
           kprintf("No more PIT counters available\n"); 
    }
    
    char write[512*10] = "WOOOOO";
    for(int i = 0; i < 1024*1024/512; i++) {
        ideWriteSectors(0, 1, 512*10, write); 
        char read[512*10];
        kmemset(read, 0, sizeof(read));
        ideReadSectors(0, 1, 512*10, read);
    }

    PITResult num_millis_elapsed = pitGetCounterCount(counter_id);
    if(num_millis_elapsed.isError == true)
        kprintf("Error getting pit counter\n");
    kprintf("Millis Elapsed: %d\n", num_millis_elapsed.count);

    
	kShellStart();
}
