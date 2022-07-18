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
#include <syscall.h>
#include <tio.h>

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

extern void jump_to_ring3(void* function_ptr);
extern void jump_to_user_mode();

void user_mode_func_test() {
    char* test = "test str\0ingsaasas";
    terminal_writ/*e(test);
    while(1);
    return;
}

void kernelMain(MultibootInfo* multiboot_info, uint32_t magic) {
    
    // Get RAM info from GRUB
    if (magic != 0x2BADB002) {
        return;
    }
    
    tio_init();
    
	/* Set up IDT */
	kprintf("Setting up the IDT\n");
	idtInit();
    
	/* Set up GDT */	
	kprintf("Setting up GDT\n");
	gdtInit();
    
	/* Init Timer and PIT */
    kprintf("Initializing PIT Timer\n");
    // TODO(alex): Check functionality
	initPITTimer();
    
    // Inform the memory unit of our physical memory situation
    loadPhysicalMemoryRegionDescriptors(multiboot_info);
    
    // Now, setup paging so we can use our physical memory
    setupPaging();
    
    syscalls_init();
    
    kheapInit();
    
	loadCpuid();
	cpuidPrintVendor();
    
	//pciCheckAllBuses();
    
    kprintf("user_mode_func_test: 0x%x\n", user_mode_func_test);
    kprintf("jump_to_ring3: 0x%x\n", jump_to_ring3);
    
    bool ide_initialized = ideInit();
    
    jump_to_ring3(user_mode_func_test);
    
	//kShellStart();
}
