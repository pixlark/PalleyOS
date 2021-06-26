#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <idt.h>
#include <exceptions.h>
#include <io.h>
#include <kstdio.h>

/* ===== PIC INITIALIZATION DEFINES ===== */
#define PIC1			0x20	/* IO base Address for master PIC */
#define PIC_EOI			0x20
#define PIC2			0xA0	/* IO base Address for slave PIC */
#define PIC1_COMMAND 	PIC1
#define PIC1_DATA		(PIC1+1)
#define PIC2_COMMAND	PIC2
#define PIC2_DATA		(PIC2+1)

#define ICW1	        0x11
#define ICW4	        0x01
#define ICW1_ICW4	    0x01		/* ICW4 (not) needed */
#define ICW1_SINGLE	    0x02		/* Single (cascade) mode */
#define ICW1_INTERVAL4	0x04	    /* Call address interval 4 (8) */
#define ICW1_LEVEL	    0x08		/* Level triggered (edge) mode */
#define ICW1_INIT	    0x10		/* Initialization - required! */

#define ICW4_8086	    0x01		/* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO	    0x02		/* Auto (normal) EOI */
#define ICW4_BUF_SLAVE	0x08		/* Buffered mode/slave */
#define ICW4_BUF_MASTER	0x0C		/* Buffered mode/master */
#define ICW4_SFNM	    0x10		/* Special fully nested (not) */


struct IDTEntry {
   uint16_t offset_1; // offset bits 0..15
   uint16_t selector; // a code segment selector in GDT or LDT
   uint8_t  zero;     // unused, set to 0
   uint8_t  type_attr;// type and attributes, see below
   uint16_t offset_2; // offset bits 16..31
} __attribute__((packed));

struct IDTInfo {
	uint16_t size;	  // Size of the table - 1
	uint32_t idt_addr; // Virtual Address of the Table
} __attribute__((packed));

typedef struct IDTInfo IDTInfo;
typedef struct IDTEntry IDTEntry;

IDTEntry idt_entries[256];
IDTInfo idt_info;

// Function that takes in a pointer to a
// IDTInfo struct and loads the idt 
static void idtLoad(IDTInfo* idt_loc) {
    __asm__ ("lidt (%0)" :: "a" (idt_loc));
}

void remapPIC(int offset1, int offset2);

bool addIsrToIdt(int num, void (*func_ptr)(), int desc_level, int gate_type){
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

// Sets a bit in the PIC, effectively telling the CPU not to listen
// to interrupts from that line
void irqSetMask(uint8_t irq_line) {
	uint16_t port;

	if(irq_line < 8)
		port = PIC1_DATA;
	else {
		port = PIC2_DATA;
		irq_line -= 8;
	}
	uint8_t value = inb(port) | (1 << irq_line);
	outb(port, value);
}

// Clears a bit in the PIC, telling the CPU to listen to that irq_line
void irqClearMask(uint8_t irq_line) {
    uint16_t port;
 
    if(irq_line < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq_line -= 8;
    }
    
    uint8_t value = inb(port) & ~(1 << irq_line);
    outb(port, value);        
}

/* ===== Interrupts ===== */
// Timer IRQ (PIT)
extern void PITIRQ();
// Keyboard Input (PC/2)
extern void keyboardIsr(void);
// Ide Controller DMA IRQ
extern void ideIRQISR(void);


/* ======== SETUP ======== */
void idtInit() {
	cli();
	remapPIC(0x20, 0x28);

	idt_info.size = (uint16_t)(sizeof(struct IDTEntry)*256) - 1;
	idt_info.idt_addr = (uintptr_t) &idt_entries;

    addExceptionsHandlersToIdt();

	addIsrToIdt(0x20, &PITIRQ, 0, INTERRUPT_GATE_32);
    addIsrToIdt(0x21, &keyboardIsr, 0, INTERRUPT_GATE_32);	

    // Primary ATA Device after DMA transfer
    addIsrToIdt(0x20 + 14, &ideIRQISR, 0, INTERRUPT_GATE_32);
    // Secondary ATA Device after DMA transfer
    addIsrToIdt(0x20 + 15, &ideIRQISR, 0, INTERRUPT_GATE_32);

    // Loads the location of the idt into the proper CPU register
	idtLoad(&idt_info);

	outb(PIC1_DATA, 0x3C);
	outb(PIC2_DATA, 0xFF);

	kprintf("idt_entries loc: 0x%x\n", idt_entries);
	sti();
}

// Adds an Interrupt Service Routine (ISR) to the Interrupt Descriptor Table (IDT)
//  - offset is the entry number in the IDT
//  - isr is a function pointer to the ISR
//  - desc_level is the ring level for the interrupt
//  - type can be 
//      - TASK_GATE
//      - INTERRUPT_GATE_16, INTERRUPT_GATE_32
//      - TRAP_GATE_16, TRAP_GATE_32
void idtAddISR(uint8_t offset, void (*isr)(), uint8_t desc_level, uint8_t type){
    cli();

    addIsrToIdt(offset, isr, desc_level, type);
    idtLoad(&idt_info);
    sti();
}

/*
 We need to remap the PIC, because the original placing of it (0x0-0xF)
 Interferes with the exception handling (Design Flaw)

 offset1 - vector offset for master PIC
 		vectors on the master will become offset1...offset1+7
 offset2 - vector offset for master PIC
 		vectors on the slave PIC will become offset2..offset2+7
*/
void remapPIC(int offset1, int offset2) {
	// Save Masks
	outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4); // Start init sequence
	ioWait();
	outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
	ioWait();

	/* The init sequence looks for three other "configuration commands"
	   Basically just reads three more bytes */

	outb(PIC1_DATA, offset1);	// ICW2: Master PIC vector offset
	ioWait();

	outb(PIC2_DATA, offset2);	// ICW2: Slave PIC vector offset
	ioWait();

	outb(PIC1_DATA, 4); 		// ICW3: tell Master PIC that there
								// is a slave PIC at IRQ2 (0000 0100)
	ioWait();

	outb(PIC2_DATA, 2);			// ICW3: Tell slave PIC its cascade identity
	ioWait();

	outb(PIC1_DATA, ICW4_8086);	// ICW4: tells other info about the environment
	ioWait();
	outb(PIC2_DATA, ICW4_8086);
	ioWait();

	outb(PIC1_DATA, 0xFC);
	outb(PIC2_DATA, 0xFF);
}
