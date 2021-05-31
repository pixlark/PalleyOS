#include <stdint.h>
#include <stddef.h>

#include <idt.h>
#include <tio.h>

#define PIC1			0x20	/* IO base Address for master PIC */
#define PIC_EOI			0x20
#define PIC2			0xA0	/* IO base Address for slave PIC */
#define PIC1_COMMAND 	PIC1
#define PIC1_DATA		(PIC1+1)
#define PIC2_COMMAND	PIC2
#define PIC2_DATA		(PIC2+1)


#define ICW1_ICW4	0x01		/* ICW4 (not) needed */
#define ICW1_SINGLE	0x02		/* Single (cascade) mode */
#define ICW1_INTERVAL4	0x04		/* Call address interval 4 (8) */
#define ICW1_LEVEL	0x08		/* Level triggered (edge) mode */
#define ICW1_INIT	0x10		/* Initialization - required! */

#define ICW4_8086	0x01		/* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO	0x02		/* Auto (normal) EOI */
#define ICW4_BUF_SLAVE	0x08		/* Buffered mode/slave */
#define ICW4_BUF_MASTER	0x0C		/* Buffered mode/master */
#define ICW4_SFNM	0x10		/* Special fully nested (not) */


struct IDTEntry {
   uint16_t offset_1; // offset bits 0..15
   uint16_t selector; // a code segment selector in GDT or LDT
   uint8_t  zero;     // unused, set to 0
   uint8_t  type_attr;// type and attributes, see below
   uint16_t offset_2; // offset bits 16..31
};

struct IDTInfo {
	uint16_t size;	  // Size of the table - 1
	uint32_t offset; // Virtual Address of the Table
};

extern void load_idt(struct IDTInfo* idt_info);
extern void keyboard_isr(void);

struct IDTEntry idt_entries[256];

struct IDTInfo idt_info;

static inline void outb(uint16_t port, uint8_t val);

void handle_idt_setup() {
	// PIC init
	outb(PIC1_DATA, 0xFF);
	outb(PIC2_DATA, 0xFF);


	idt_info.size = sizeof(idt_entries) - 1;
	idt_info.offset = (uint32_t) idt_entries;

	// Adding Keyboard ISR
	void (*key_isr_ptr)() = &keyboard_isr;	
	struct IDTEntry keyboard_interrupt;
	keyboard_interrupt.offset_1 = (uint32_t)key_isr_ptr & 0xffff;
	keyboard_interrupt.selector = 0x0008;
	keyboard_interrupt.zero = 0;
	keyboard_interrupt.type_attr = 0b1010 << 4 | 0b1110; // 0b1110 means interrupt gate
	keyboard_interrupt.offset_2 = (uint32_t)key_isr_ptr & ~(0xffff);
	idt_entries[1] = keyboard_interrupt;
	
	load_idt(&idt_info);

	term_write("idt_entries loc: ");
	term_write_uint32((uint32_t) idt_entries, 16);
	term_write("\n");
}

void keyboard_interrupt_handler() {
	term_write("In Keyboard Interrupt\n");	
}

/* I/O Assembly */
static inline void outb(uint16_t port, uint8_t val) {
	asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

static inline uint8_t inb(uint16_t port) {
	uint8_t ret;
	asm volatile ( "inb %1, %0" 
					: "=a"(ret)
					: "Nd"(port) );
	return ret;
}

static inline void io_wait(void) {
	asm volatile ( "jmp 1f\n\t"
					"1: jmp 2f\n\t"
					"2:" );
}

static are_interrupts_enabled() {
	unsigned long flags;
	asm volatile ( "pushf\n\t"
					"pop %0"
					: "=g"(flags) );
	return flags & (1 << 9);
}

void PIC_sendEOI(unsigned char irq) {
	if(irq >= 8)
		outb(PIC2_COMMAND, PIC_EOI);
	outb(PIC1_COMMAND, PIC_EOI);
}

/*
arguments:
	offset1 - vector offset for master PIC
		vectors on the master become offset1..offset1+7
	offset2 - same for slave PIC: offset2..offset2+7
*/
void PIC_remap(int offset1, int offset2)
{
	unsigned char a1, a2;
 
	a1 = inb(PIC1_DATA);                        // save masks
	a2 = inb(PIC2_DATA);
 
	outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);  // starts the initialization sequence (in cascade mode)
	io_wait();
	outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
	io_wait();
	outb(PIC1_DATA, offset1);                 // ICW2: Master PIC vector offset
	io_wait();
	outb(PIC2_DATA, offset2);                 // ICW2: Slave PIC vector offset
	io_wait();
	outb(PIC1_DATA, 4);                       // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
	io_wait();
	outb(PIC2_DATA, 2);                       // ICW3: tell Slave PIC its cascade identity (0000 0010)
	io_wait();
 
	outb(PIC1_DATA, ICW4_8086);
	io_wait();
	outb(PIC2_DATA, ICW4_8086);
	io_wait();
 
	outb(PIC1_DATA, a1);   // restore saved masks.
	outb(PIC2_DATA, a2);
}
