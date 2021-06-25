#pragma once
#include <stdbool.h>

#define TASK_GATE			0x5
#define INTERRUPT_GATE_16	0x6
#define TRAP_GATE_16		0x7
#define INTERRUPT_GATE_32	0xE
#define TRAP_GATE_32		0xF

void idtHandleSetup();
bool addIsrToIdt(int num, void (*func_ptr)(), int desc_level, int gate_type);

