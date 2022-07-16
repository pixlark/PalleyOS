
#include <syscall.h>
#include <idt.h>
#include <kstdio.h>

struct Registers {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t esi;
    uint32_t edi;
};
typedef struct Registers Registers;

extern void syscallIsr();
static void syscallHandler();

uint32_t num_syscalls = 1;
static void* syscalls[1] = 
{
    &kprintf,
};

void syscallsInit() {
    kprintf("INIT SYSCALLS\n");
    idtAddISR(0x80, &syscallIsr, 3, INTERRUPT_GATE_32);
}

// push param_n
// push param...
// push param_1
// push syscall_index
// int $0x80

// Called from syscallIsr
//void syscallHandler(uint32_t syscall_index) {
//
//    if(syscall_index >= num_syscalls)
//        return;
//}
