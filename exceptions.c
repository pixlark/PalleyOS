#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include <kstdio.h>
#include <idt.h>

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
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
extern void pageFaultIsr();
extern void handle_page_fault();
void pageFaultHandler(uint32_t err) {
    // TODO(Paul): Check error code for error type
    handle_page_fault();
}
#pragma GCC diagnostic push

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

void addExceptionsHandlersToIdt() {

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

}


