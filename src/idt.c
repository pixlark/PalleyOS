#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <idt.h>
#include <tio.h>
#include <io.h>
#include <kstdio.h>

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

// Located in boot.s
extern void idtLoad(void);

struct IDTEntry idt_entries[256];
struct IDTInfo idt_info;

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

void irqSetMask(uint8_t irq_line) {
	uint16_t port;
	uint8_t value;

	if(irq_line < 8)
		port = PIC1_DATA;
	else {
		port = PIC2_DATA;
		irq_line -= 8;
	}
	value = inb(port) | (1 << irq_line);
	outb(port, value);
}

void irqClearMask(uint8_t irq_line) {
    uint16_t port;
    uint8_t value;
 
    if(irq_line < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq_line -= 8;
    }
    value = inb(port) & ~(1 << irq_line);
    outb(port, value);        
}

/* ===== EXCEPTIONS ===== */
// Divide by Zero (Fault) (0)
extern void divByZeroIsr(void);
void divByZeroHandler() {
	kprintf("Divide By Zero Fault\n");
	while(true);
}

// Debug (Fault/Trap) (1)
extern void debugIsr(void);
void debugHandler() {
	kprintf("Debug Fault/Trap\n");
	while(true);
}

// Breakpoint (Trap) (3)
extern void breakpointIsr(void);
void breakpointHandler() {
	kprintf("Breakpoint Trap\n");
	while(true);
}

// Overflow (Trap) (4)
extern void overflowIsr(void);
void overflowHandler() {
	kprintf("Overflow occured\n");
}

// Bound Range Exceeded Fault (5)
extern void boundRangeIsr();
void boundRangeExceededHandler(){
	kprintf("Out of bounds fault\n");
	while(true);
}

// Invalid Opcode Fault (6)
extern void invalidOpcodeIsr(void);
void invalidOpcodeHandler() {
	kprintf("Invalid Opcode!\n");
	while(true);
}

// Device Not Available Fault (7)
extern void deviceNAIsr(void);
void deviceNAHandler() {
	kprintf("Device Not Available Fault\n");
	while(true);
}

// Double Fault (Abort) (8)
extern void doubleFaultIsr();
void doubleFaultHandler() {
	kprintf("Double fault\n");
	while(true);
}

// Invalid TSS Fault (10)
extern void invalidTSSIsr();
void invalidTSSHandler(uint32_t err) {
	kprintf("Invalid TSS Fault, error code: %d\n", err);
	while(true);	
}

// Segment Not Present Fault (11)
extern void segNotPresIsr();
void segNotPresHandler(uint32_t err) {
	kprintf("Segment Not Present Fault, error code: %d\n", err);	
	while(true);
}

// Stack Segment Fault (12)
extern void stackSegIsr();
void stackSegHandler(uint32_t err) {
	kprintf("Stack Segmentation Fault, error code: %d\n", err);	
	while(true);
}

// General Protection Fault (13)
extern void generalProtFaultIsr();
void generalProtFaultHandler(uint32_t err) {
	kprintf("General Protection Fault, error code: 0b%b\n", err);
	kprintf("Table: %s, Selector Index: %d\n", 
			(const char*[]){"GDT", "IDT", "LDT", "IDT"}[(err&0x6)>>1], (err & ~(7))>>3);
	while(true);	
}

// Page Fault (14)
extern void pageFaultIsr();
extern void handle_page_fault();
// Stack
// err
// iret = the place where we messed up
void pageFaultHandler(uint32_t err) {
    // TODO(Paul): Check error code for error type
    handle_page_fault();
}

// x87 Floating-Point Exception (Fault) (16)
extern void fpeIsr();
void fpeHandler() {
	kprintf("x87 Floating Point Exception (Fault)\n");
	while(true);	
}

// Alignment Check Fault (17) 
extern void alignCheckIsr();
void align_check_handler(uint32_t err) {
	kprintf("Alignment Check Fault, error code: %d\n", err);
	while(true);	
}

// Machine Check (18)
extern void machineCheckIsr();
void machine_check_handler() {
	kprintf("Machine Check (Abort)");
	while(true);	
}

// SIMD Floating point Exception (Fault) (19)
extern void simdFpeIsr();
void simdFpeHandler() {
	kprintf("SIMD Floating point Exception (Fault)");
	while(true);	
}

// Virtualization Exception (20)
extern void virtIsr();
void virtHandler() {
	kprintf("Virtualization Exception (Fault)\n");
	while(true);	
}

/* ====== Interrupts ===== */

// Timer IRQ (PIT)
extern void TimerIRQ();

// Keyboard Input (PC/2)
extern void keyboardIsr(void);

void initPIC(int, int);

/* ======== SETUP ======== */
void idtHandleSetup() {
	cli();
	initPIC(0x20, 0x28);

	idt_info.size = (uint16_t)(sizeof(struct IDTEntry)*256) - 1;
	idt_info.offset = (uint32_t) &idt_entries;

	addIsrToIdt(0, &divByZeroIsr, 0, TRAP_GATE_32);
	addIsrToIdt(1, &debugIsr, 0, TRAP_GATE_32);
	addIsrToIdt(3, &breakpointIsr, 0, TRAP_GATE_32);
	addIsrToIdt(4, &overflowIsr, 0, TRAP_GATE_32);
	addIsrToIdt(5, &boundRangeIsr, 0, TRAP_GATE_32);
	addIsrToIdt(6, &invalidOpcodeIsr, 0, TRAP_GATE_32);
	addIsrToIdt(7, &deviceNAIsr, 0, TRAP_GATE_32);
	addIsrToIdt(8, &doubleFaultIsr, 0, TRAP_GATE_32);
	addIsrToIdt(10, &invalidTSSIsr, 0, TRAP_GATE_32);
	addIsrToIdt(11, &segNotPresIsr, 0, TRAP_GATE_32);
	addIsrToIdt(12, &stackSegIsr, 0, TRAP_GATE_32);
	addIsrToIdt(13, &generalProtFaultIsr, 0, TRAP_GATE_32);
	addIsrToIdt(14, &pageFaultIsr, 0, TRAP_GATE_32);
	addIsrToIdt(16, &fpeIsr, 0, TRAP_GATE_32);
	addIsrToIdt(17, &alignCheckIsr, 0, TRAP_GATE_32);
	addIsrToIdt(18, &machineCheckIsr, 0, TRAP_GATE_32);
	addIsrToIdt(19, &simdFpeIsr, 0, TRAP_GATE_32);
	addIsrToIdt(20, &virtIsr, 0, TRAP_GATE_32);

	addIsrToIdt(0x20, &TimerIRQ, 0, INTERRUPT_GATE_32);
	for(int i = 0x21; i < 0x20+16; i++)
		addIsrToIdt(i, &keyboardIsr, 0, INTERRUPT_GATE_32);	

	idtLoad();

	outb(PIC1_DATA, 0xFC);
	outb(PIC2_DATA, 0xFF);

	kprintf("idt_entries loc: 0x%x\n", idt_entries);
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
void initPIC(int offset1, int offset2) {
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
