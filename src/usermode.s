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
mov $0x100008, %eax
mov $0x174, %ecx
wrmsr

mov 4(%esp),%edx # User function to go to

# should save kernel stack bsp somewhere
#mov $0x02000000, %ecx
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
push user_mode_func_test
iret

.global func
func:
#cli
jmp .
