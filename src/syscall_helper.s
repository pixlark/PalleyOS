.extern syscall_try_call
.global syscall_isr
.type syscall_isr, @function
syscall_isr:
push %ebp
mov %esp, %ebp 

# return address  = 0x4 bytes
# 0x20 (32) bytes for pushed registers
# int (interrupt) command puts 0xC (12) bytes of info on stack
# return address to syscall function = 0x4 bytes
# 0x20 + 0x14 = 0x34 total bytes before argument passed into syscall();

mov %ebp, %eax
add $0x38, %eax
push %eax # pass pointer to first argument (va_list)

mov 0x34(%ebp), %eax  # pass syscall_index
push %eax
call syscall_try_call
mov %ebp, %esp
pop %ebp
iret


.global syscall
.type syscall, @function
syscall:
pushal
cld
int $0x80 # must be same number as idt index for syscall
popal
ret