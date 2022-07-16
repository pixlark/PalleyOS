
.extern syscallHandler
.global syscallIsr
.type syscallIsr, @function
syscallIsr:
pushal
cld
call syscallHandler
popal
iret

#void* syscalls[]
.extern syscalls
.extern num_syscalls
syscallHandler:

# Example syscall
# push param1
# push param...
# push param_n
# push syscall_index
# int 0x80

# check if syscall_index is valid
early_return:
ret

//.global userModeFunc
//.type userModeFunc, @function
//userModeFunc:
//pushl $0xAA
//pushl $0xBB
//pushl $0xCC
//pushl $0xDD
//pushl $0xEF
//int $0x80
//pop %eax
//pop %eax
//pop %eax
//pop %eax
//pop %eax
//ret

