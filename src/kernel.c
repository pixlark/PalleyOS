#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <ata.h>
#include <cpuid.h>
#include <gdt.h>
#include <idt.h>
#include <io.h>
#include <kheap.h>
#include <kstdio.h>
#include <kstdlib.h>
#include <memory.h>
#include <pci.h>
#include <pic.h>
#include <sknyfs.h>
#include <terminal_proc.h>
#include <timer.h>
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

	/* Init Timer and PIT */
	//initPITTimer();

	//loadCpuid();
	//cpuidPrintVendor();

	//pciCheckAllBuses();

    // Ensure there's a hard drive loaded
    if (!ide_devices[0].reserved) {
        kprintf("No hard drive loaded!\n");
        while (true);
    }
    if (ide_devices[0].type != 0) {
        kprintf("Hard drive is not ATA type!\n");
        while (true);
    }

    {
        uint8_t drive = 0;
        SknyHandle handle;
        SknyStatus status;
        if ((status = sknyCreateFilesystem(&handle, drive)) != SKNY_STATUS_OK) {
            kprintf("(!) Error formatting filesystem!\n  %s\n", sknyStatusToString(status));
            while (true);
        }
        for (int i = 0; i < 100; i++) {
            if ((status = sknyCreateFile(&handle, "blah.txt")) != SKNY_STATUS_OK) {
                kprintf("(!) Error creating file #%d!\n  %s\n", i, sknyStatusToString(status));
                while(true);
            }
        }
    }

	terminal_proc_start();

	kShellStart();
}
