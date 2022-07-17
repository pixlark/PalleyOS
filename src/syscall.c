
#include <syscall.h>
#include <idt.h>
#include <kstdio.h>

typedef struct Registers {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t esi;
    uint32_t edi;
} Registers;

extern void syscall_isr();


typedef struct {
    size_t len;
    char data[];
} String;


void syscalls_init() {
    kprintf("INIT SYSCALLS\n");
    idt_add_isr(0x80, syscall_isr, 3, INTERRUPT_GATE_32);
}


