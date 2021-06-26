#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define TASK_GATE			0x5
#define INTERRUPT_GATE_16	0x6
#define TRAP_GATE_16		0x7
#define INTERRUPT_GATE_32	0xE
#define TRAP_GATE_32		0xF

// Initializes the Interrupt Descriptor Table with basic interrupts such as
// exceptions and keyboard ISR
void idtInit();

// Adds an Interrupt Service Routine (ISR) to the Interrupt Descriptor Table (IDT)
//  - offset is the entry number in the IDT
//  - isr is a function pointer to the ISR
//  - desc_level is the ring level for the interrupt
//  - type can be 
//      - TASK_GATE
//      - INTERRUPT_GATE_16, INTERRUPT_GATE_32
//      - TRAP_GATE_16, TRAP_GATE_32
void idtAddISR(uint8_t id, void (*isr)(), uint8_t desc_level, uint8_t type);

