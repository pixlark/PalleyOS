.extern syscall_try_call
.global syscall_isr
.type syscall_isr, @function

# MODIFIES: eax
syscall_isr:
push %ebp
mov %esp, %ebp 

cmp $0x0, %eax
jne next_sys_call
push (%ebx)
call tio_write
jmp end

next_sys_call:

end:
mov %ebp, %esp
pop %ebp
iret



.global terminal_write
.type terminal_write, @function
terminal_write:
push %ebp
mov %esp, %ebp
pushal
cld

mov 0x8(%ebp), %ebx
mov $0x0, %eax
int $0x80

popal
mov %ebp, %esp
pop %ebp
ret

