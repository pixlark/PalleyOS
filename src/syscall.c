
#include <syscall.h>
#include <idt.h>
#include <kstdio.h>

typedef void (*Syscall_Func_Ptr)(va_list args);

typedef struct Registers {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t esi;
    uint32_t edi;
} Registers;

extern void syscall_isr();

void syscall_printf(va_list args);

uint32_t num_syscalls = 1;
static Syscall_Func_Ptr syscalls[0x20];

void syscalls_init() {
    kprintf("INIT SYSCALLS\n");
    idt_add_isr(0x80, syscall_isr, 3, INTERRUPT_GATE_32);
    syscalls[0x16] = syscall_printf;
}

void syscall_try_call(int index, va_list args){
    if(syscalls[index] != 0) {
        // TODO(alex): somehow implement error checking to make sure
        // the syscall includes correct number of arguments
        syscalls[index](args);
    }
}

SYSCALL(syscall_printf){
    const char* format = va_arg(args, const char*);
    kvprintf(format, args);
}

