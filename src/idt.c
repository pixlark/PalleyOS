#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <idt.h>
#include <tio.h>
#include <io.h>

#define PIC1			0x20	/* IO base Address for master PIC */
#define PIC_EOI			0x20
#define PIC2			0xA0	/* IO base Address for slave PIC */
#define PIC1_COMMAND 	PIC1
#define PIC1_DATA		(PIC1+1)
#define PIC2_COMMAND	PIC2
#define PIC2_DATA		(PIC2+1)


#define ICW1	0x11
#define ICW4	0x01
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

#define TASK_GATE			0x5
#define INTERRUPT_GATE_16	0x6
#define TRAP_GATE_16		0x7
#define INTERRUPT_GATE_32	0xE
#define TRAP_GATE_32		0xF


struct IDTEntry {
   uint16_t offset_1; // offset bits 0..15
   uint16_t selector; // a code segment selector in GDT or LDT
   uint8_t  zero;     // unused, set to 0
   uint8_t  type_attr;// type and attributes, see below
   uint16_t offset_2; // offset bits 16..31
} __attribute__((packed));

struct IDTInfo {
	uint16_t size;	  // Size of the table - 1
	uint32_t offset; // Virtual Address of the Table
} __attribute__((packed));

extern void load_idt(void);

struct IDTEntry idt_entries[256];
struct IDTInfo idt_info;

static inline void outb(uint16_t port, uint8_t val);

bool add_isr_to_idt(int num, void (*func_ptr)(), int desc_level, int gate_type){
	if(num < 0) {
		return false;	
	}

	idt_entries[num].offset_1 = ((uint32_t)func_ptr & 0xffff);
	idt_entries[num].offset_2 = ((uint32_t)func_ptr & 0xffff0000) >> 16;

	idt_entries[num].zero = 0;
	idt_entries[num].type_attr = (desc_level & 0x3) << 5 | (1 << 7);
	idt_entries[num].type_attr |= gate_type & 0xf;
	idt_entries[num].selector = 0x08;

	if(gate_type == INTERRUPT_GATE_16 || gate_type == INTERRUPT_GATE_32
		|| gate_type == TRAP_GATE_16 || gate_type == TRAP_GATE_32)	
		idt_entries[num].type_attr &= ~(1 << 4); 
	else
		idt_entries[num].type_attr |= (1 << 4); 

	return true;
}

extern void test_isr(void);
void test_interrupt_handler(){
	term_write("Inside interrupt!\n");	
}

extern void keyboard_isr(void);
void keyboard_interrupt_handler(){
	term_write("key pressed!\n");
}

extern void invalid_opcode_isr(void);
void invalid_opcode_handler(uint32_t in) {
	term_write("Invalid Opcode!\n");
	term_write("eip: ");
	term_write_uint32(in, 16);
	term_write("\n");
	while(true);
}

extern void bound_range_isr();
void bound_range_exceeded_handler(){
	term_write("Out of bounds fault\n");
	while(true);
}

extern void IRQ0_handler();

void handle_idt_setup() {
	disable_interrupts();
	init_pics(0x20, 0x28);
	enable_interrupts();

	idt_info.size = (uint16_t)(sizeof(struct IDTEntry)*256) - 1;
	idt_info.offset = (uint32_t) &idt_entries;

	add_isr_to_idt(0, &IRQ0_handler, 0, INTERRUPT_GATE_32);
	for(int i = 0; i < 256; i++)
		add_isr_to_idt(i, &keyboard_isr, 0, INTERRUPT_GATE_32);	
	add_isr_to_idt(5, &bound_range_isr, 0, TRAP_GATE_32);
	add_isr_to_idt(6, &invalid_opcode_isr, 0, TRAP_GATE_32);

	if(!add_isr_to_idt(52, &test_isr, 0, INTERRUPT_GATE_32)) {
		term_write("unable to add isr\n");
	}

	load_idt();

	term_write("idt_entries loc: ");
	term_write_uint32((uint32_t) idt_entries, 16);
	term_write("\n");

}
/*
 We need to remap the PIC, because the original placing of it (0x0-0xF)
 Interferes with the exception handling (Design Flaw)

 offset1 - vector offset for master PIC
 		vectors on the master will become offset1...offset1+7
 offset2 - vector offset for master PIC
 		vectors on the slave PIC will become offset2..offset2+7
*/
void init_PIC(int offset1, int offset2) {
	// Save Masks
	outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4); // Start init sequence
	io_wait();
	outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
	io_wait();

	outb(PIC1_DATA, offset1);	// ICW2: Master PIC vector offset
	io_wait();

	outb(PIC2_DATA, offset2);	// ICW2: Slave PIC vector offset
	io_wait();

	outb(PIC1_DATA, 4); 		// ICW3: tell Master PIC that there
								// is a slave PIC at IRQ2 (0000 0100)
	io_wait();

	outb(PIC2_DATA, 2);			// ICW3: Tell slave PIC its cascade identity
	io_wait();

	outb(PIC1_DATA, ICW4_8086);
	io_wait();
	outb(PIC2_DATA, ICW4_8086);
	io_wait();

	outb(PIC1_DATA, 0x00);
	outb(PIC2_DATA, 0x00);
}

void init_pics(int pic1, int pic2)
{
   /* send ICW1 */
   outb(PIC1, ICW1);
   outb(PIC2, ICW1);

   /* send ICW2 */
   outb(PIC1 + 1, pic1);   
   outb(PIC2 + 1, pic2);   

   /* send ICW3 */
   outb(PIC1 + 1, 4);   
   outb(PIC2 + 1, 2);

   /* send ICW4 */
   outb(PIC1 + 1, ICW4);
   outb(PIC2 + 1, ICW4);

   /* disable all IRQs */
   outb(PIC1 + 1, 0x00);
}
