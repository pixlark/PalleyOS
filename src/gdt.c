/*
 * The Global Descriptor Table (GDT) is specific to the IA32 Architecture. It contains entries which tell the CPU
 * about memory segments
 */
#include <stddef.h>
#include <stdint.h>

#include <gdt.h>
#include <kstdio.h>

struct GDTEntry {
   uint16_t limit_low;
   uint16_t base_low;
   uint8_t  base_middle;
   uint8_t  access;
   uint8_t  granularity;
   uint8_t  base_high;
} __attribute__((packed));

typedef struct GDTEntry GDTEntry;

struct GDTPtr {
	uint16_t limit;
	uint32_t base;
}__attribute__((packed));

typedef struct GDTPtr GDTPtr;

extern void gdtFlush();
struct GDTEntry GDT[3];
GDTPtr gp;

// Sets an entry in the GDT
void gdtSetGate(int num, uint32_t base, uint32_t limit,
						   uint8_t access, uint8_t gran) {
	/* Set up descriptor base address */
	GDT[num].base_low = (base & 0xffff);
	GDT[num].base_middle = (base >> 16) & 0xff;
	GDT[num].base_high = (base >> 24) & 0xff;

	/* Set up descriptor limits */
	GDT[num].limit_low = (limit & 0xffff);
	GDT[num].granularity = ((limit >> 16) & 0x0f);

	/* Set up the granularity and access flags */
	GDT[num].granularity |= (gran & 0xF0);
	GDT[num].access = access;
}


void gdtInit() {
	// TODO: Change so half of memory is for kernel, other half for user

    // The first entry must be NULL
	gdtSetGate(0, 0, 0, 0, 0);

	/* Base Adress: 0
	 * Limit is 4GiB
	 * 4 KByte Granularity
	 * Uses 32-bit opcodes
	 * Code Segment Descriptor
	 */
	gdtSetGate(1, 0, 0xffffffff, 0x9A, 0xCF); 

	/* Data Segment Descriptor
	 * Same as above but descriptor type is DS
	 */
	gdtSetGate(2, 0, 0xffffffff, 0x92, 0xCF);

	kprintf("GDT loc: 0x%x\n", &GDT);

	gp.limit = (sizeof(struct GDTEntry) * 3) - 1;
	gp.base = (uintptr_t)&GDT;
	
	/* Flush out the old GDT and install the new changes */
	gdtFlush();
}

