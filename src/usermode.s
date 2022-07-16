.section .text

.global jump_to_ring3
.type jump_to_ring3, @function
.extern userModeFunc
# param: uint32_t function_ptr
jump_to_ring3:
mov $0x23, %ax
mov %ax, %ds
mov %ax, %es
mov %ax, %fs
mov %ax, %gs

xor %edx, %edx
mov $0x10008, %eax
mov $0x174, %ecx
wrmsr

#movl 8(%esp),%ebx # User function to go to
movl user_mode_func_test, %ebx

mov %ebx, %edx
mov %esp, %ecx
sysexit

.global jump_to_user_mode
.type jump_to_user_mode, @function
.extern user_mode_func_test
# param: uint32_t function_ptr
jump_to_user_mode:
mov $0x23, %ax
mov %ax, %ds
mov %ax, %es
mov %ax, %fs
mov %ax, %gs

mov %esp, %eax
push $0x23
push %eax
pushf
push (3*8)| 3
push user_mode_func_test
iret


# This is deprecated (I couldn't make it work).
# This uses the TSS, which is what I couldn't get to work lmao
#uint32_t param for function ptr
jump_to_ring3_dep:
cli
mov $0x23, %ax # ring 3 data with bottom 2 priviledge bits set for ring 3
mov %ax, %ds
mov %ax, %es
mov %ax, %fs
mov %ax, %gs

movl 8(%esp), %ebx

# Set up the stack frame that iret expects
mov %esp, %eax
push $0x23 # data sector offset in GDT
push %eax
pushf # eflags

pop %eax
or $0x200, %eax # Turn interrupts back on
push %eax

pushl $0x1b # code sector offset in GDT
push $1f
iret

1:
jmp .



.global jump_to_ring0
.type jump_to_ring0, @function
jump_to_ring0:
mov $0x10, %ax # ring 0 data with bottom 2 priviledge bits set for ring 0
mov %ax, %ds
mov %ax, %es
mov %ax, %fs
mov %ax, %gs

# Set up the stack frame that iret expects
mov %esp, %eax
push $0x10
push %eax
pushf # eflags
push $0x1b
push userModeFunc
iret

.global func
func:
#cli
jmp .
