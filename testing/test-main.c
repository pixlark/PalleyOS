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
#include <kshell.h>
#include <memory.h>
#include <idt.h>
#include <kheap.h>
#include <timer.h>


void testATA() {

    char toWrite[512];
    kmemset(toWrite, 0xBA, 512);

    uint32_t sects_per_GB = 1024*1024*1024/512;
    int num_GB = 3;
    for(int j = 0; j < num_GB; j++) {
        for(uint32_t i = 0; i < sects_per_GB; i++){
            ideWriteSectors(0, 1, 512*i + sects_per_GB*j, toWrite); 

            char readFromDevice[512];
            kmemset(readFromDevice, i*512, 512);
            ideReadSectors(0, 1, 512*i, readFromDevice);

            if(!kmemcmp(readFromDevice, toWrite, 512)) {
                kprintf("DISK ERROR: Read different data than wrote, GB: %d, "
                                "sector: %d/\n", j, i, sects_per_GB);
            }
            else if(i % 50000  == 0){
                kprintf("DISK SUCCESS: GB: %d, Sector %d/%d\n", j, i, sects_per_GB);
            }
        }
    }

}

int kernelMain(MultibootInfo* multiboot_info, uint32_t magic) {
    // TODO: This stuff which exists both in this kernelMain and in
    // the normal kernelMain should be abstracted out into some kind
    // of 'setup' function
    
    // Get RAM info from GRUB
    if (magic != 0x2BADB002) {
        return 1;
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
    testATA();

	kShellStart();
    return 0;
}
