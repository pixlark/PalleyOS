/*
 * The Global Descriptor Table (GDT) is specific to the IA32 Architecture. It contains entries which tell the CPU
 * about memory segments
 */
#include <stddef.h>
#include <stdint.h>

#include <gdt.h>
#include <kstdio.h>
#include <kstdlib.h>
#include <scheduler.h>

#define GDT_DATA 0
#define GDT_CODE 1
#define GDT_4KB  1

uint8_t test_stack[1024*16] __attribute__((aligned (4)));

typedef struct {
    uint32_t limit_low               : 16;
    uint32_t base_low                : 24;
    uint32_t accessed                : 1;
    uint32_t read_write              : 1;
    uint32_t direction_conforming    : 1; // 0: the segment grows up, 1 segment grows down
    // for code sectors, 0: this segment can only be
    // executed from ring set in privl
    // 1: This code can be executed from an equal
    // or lower privilege level (see docs for more details)
    uint32_t executable             : 1;// 0: data, 1: code
    uint32_t type                   : 1;// Should be 1 for everything but TSS and LDT 
    uint32_t privilege              : 2; // Ring level
    uint32_t present                : 1;
    uint32_t limit_high             : 4;
    uint32_t available              : 1; // Used only in software, not effect on hardware
    uint32_t long_mode              : 1; // Used to indicate x86-64 code descriptor
    // For data segments, this is reserved
    uint32_t sz                     : 1; // 0: 16 protected mode, 1: 32 bit prot mode
    uint32_t granularity            : 1; // 0: byte addressing, 1: 4Kib addressing
    uint32_t base_high              : 8;
} __attribute__((packed)) GDTEntry;

typedef struct {
	uint16_t limit;
	uint32_t base;
}__attribute__((packed)) GDTPtr;

extern void gdtFlush();
extern uint32_t get_gdt_register_value(GDTPtr* out);
GDTEntry GDT[6];
GDTPtr gp;

extern void tssFlush();
TSSEntry tss_entry;

// Sets an entry in the GDT
static void gdtSetGate(int num, GDTEntry* entry) {
    GDT[num] = *entry;
}

static GDTEntry generateRing0CodeSegment() {
    GDTEntry ring0_code = {0};
    ring0_code.present = 1;
    ring0_code.privilege = 0;
    
    ring0_code.limit_low = 0xFFFF;
    ring0_code.limit_high = 0xF;
    
    ring0_code.base_low  = 0x0;
    ring0_code.base_high = 0x0;
    
    ring0_code.granularity = GDT_4KB; // We want to span the entire 4GiB
    
    ring0_code.executable = 1;
    ring0_code.type = 1;
    
    ring0_code.read_write = 1;
    ring0_code.sz = 1; // 32 bit protected mode
    ring0_code.executable = 1; // Code Sector
    
    return ring0_code;
}


static GDTEntry generateRing3CodeSegment(){
    GDTEntry ring3_code = {0};
    ring3_code.present = 1;
    
    ring3_code.privilege = 3;
    
    ring3_code.limit_low = 0xFFFF;
    ring3_code.limit_high = 0xF;
    
    ring3_code.base_low  = 0x0;
    ring3_code.base_high = 0x0;
    
    ring3_code.granularity = GDT_4KB; // We want to span the entire 4GiB
    
    ring3_code.executable = 1;
    ring3_code.type = 1;
    
    ring3_code.read_write = 1;
    ring3_code.sz = 1; // 32 bit protected mode
    ring3_code.executable = 1; // Code Sector
    
    return ring3_code;
}

static GDTEntry generateTSS() {
    uint32_t base = (uint32_t) &tss_entry;    
    uint32_t limit = sizeof(tss_entry);
    
    GDTEntry gdt_tss = {0};
    gdt_tss.limit_low = limit;
    gdt_tss.base_low = base;
    gdt_tss.accessed = 1; // With system entry ("type = 0"), 1 indicated TSS
    gdt_tss.read_write = 0; // For TSS 0: Not busy, 1: BSY
    gdt_tss.direction_conforming = 0; // Always 0 for TSS
    gdt_tss.executable = 1; // For TSS, 0: 16-bit, 1: 32-bit
    gdt_tss.type = 0; // indicates TSS/LDT
    gdt_tss.privilege = 3; // User Mode
    gdt_tss.present = 1;
    gdt_tss.limit_high = (limit & (0xf << 16)) >> 16;
    gdt_tss.available = 0;
    gdt_tss.granularity = 0;
    gdt_tss.base_high = (base & (0xff << 24)) >> 24;
    
    // Note: CS is loaded from the IDT entry and should be the regular kernel code segment
    return gdt_tss;
}

static void writeTSSEntry(uint16_t stack_segment, uint32_t kernel_stack_ptr) {
    kmemset(&tss_entry, 0, sizeof(TSSEntry));
    tss_entry.ss0 = stack_segment;
    tss_entry.esp0 = kernel_stack_ptr;
    
    tss_entry.cs = 0x0b;
    tss_entry.ss = tss_entry.ds = 0x13;
    tss_entry.es = tss_entry.fs = tss_entry.gs = 0x13;
}

// Called from an interrupt
void tssSetKernelStack (uint32_t stack) {
    tss_entry.esp0 = stack;
}

void gdtInit() {
	// TODO: Change so half of memory is for kernel, other half for user
    
    // The first entry must be NULL
    GDTEntry null_entry = {0};
	gdtSetGate(0, &null_entry);
    
    GDTEntry ring0_code_entry;
    ring0_code_entry = generateRing0CodeSegment();
    gdtSetGate(1, &ring0_code_entry);
    
    GDTEntry* ring0_data_entry;
    ring0_data_entry = &ring0_code_entry; 
    ring0_data_entry->executable = 0;
	gdtSetGate(2, ring0_data_entry);
    
    GDTEntry ring3_code_entry;
    ring3_code_entry = generateRing3CodeSegment();
	gdtSetGate(3, &ring3_code_entry);
    
    GDTEntry* ring3_data_entry;
    ring3_data_entry = &ring3_code_entry; 
    ring3_data_entry->executable = 0;
	gdtSetGate(4, ring3_data_entry);
    
    // Note!!!! If you want to change the location of the TSS (i.e. not the 5th element)
    // in the GDT, you must change tssFlush to load the Task Register with the proper
    // offset
    
    extern uint32_t stack_top;
    kprintf("stack_top: 0x%x\n", &stack_top);
    
    uint16_t kernel_stack_segment = 0x10;
    uint32_t kernel_stack_ptr = 0x0;
    
    GDTEntry gdt_tss = generateTSS();
    gdtSetGate(5, &gdt_tss);
    writeTSSEntry(kernel_stack_segment, kernel_stack_ptr);
    
	kprintf("GDT loc: 0x%x\n", &GDT);
    
	gp.limit = (sizeof(GDTEntry) * 6) - 1;
	gp.base = (uintptr_t)&GDT;
    
	/* Flush out the old GDT and install the new changes */
	gdtFlush();
    tssFlush(); // TODO(???): TSS Messed up or sumtin
    
    GDTPtr stored_gdt_loc = {};
    stored_gdt_loc.base = 0;
    stored_gdt_loc.limit = 0;
    
    get_gdt_register_value(&stored_gdt_loc);
    
    if(stored_gdt_loc.limit == 0 && stored_gdt_loc.base != gp.base) {
        kprintf("GDT Location does not match\n");
        kprintf("GDT Base: 0x%x\n", gp);
        kprintf("Stored GDT Register: 0x%x\n", stored_gdt_loc.base);
        while(1);
    }
}

