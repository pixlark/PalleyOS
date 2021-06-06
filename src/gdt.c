#include <stddef.h>
#include <stdint.h>

#include <gdt.h>
#include <tio.h>

struct GDTEntry {
   uint16_t limit_low;
   uint16_t base_low;
   uint8_t  base_middle;
   uint8_t  access;
   uint8_t  granularity;
   uint8_t  base_high;
} __attribute__((packed));

struct gdt_ptr {
	uint16_t limit;
	uint32_t base;
}__attribute__((packed));

extern void gdt_flush();
struct GDTEntry GDT[3];
struct gdt_ptr gp;

void gdt_set_gate(int num, uint32_t base, uint32_t limit,
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

void setup_gdt() {
	// Sets flat segments that span all of memory
	// TODO: Change so half of memory is for kernel, other half for user

	/* NULL Descriptor */
	gdt_set_gate(0, 0, 0, 0, 0);

	/* Base Adress: 0
	 * Limit is 4GiB
	 * 4 KByte Granularity
	 * Uses 32-bit opcodes
	 * Code Segment Descriptor
	 */
	gdt_set_gate(1, 0, 0xffffffff, 0x9A, 0xCF); 

	/* Data Segment Descriptor
	 * Same as above but descriptor type is DS
	 */
	gdt_set_gate(2, 0, 0xffffffff, 0x92, 0xCF);

	term_write("GDT loc: ");
	term_write_uint32((uint32_t)&GDT, 16);
	term_write("\n");

	gp.limit = (sizeof(struct GDTEntry) * 3) - 1;
	gp.base = (uint32_t)&GDT;
	
	/* Flush out the old GDT and install the new changes */
	gdt_flush();
}

