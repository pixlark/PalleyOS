.extern keyboard_interrupt_handler
.align 4
.global keyboard_isr
.type keyboard_isr,@function
keyboard_isr:
	call keyboard_interrupt_handler
	iret

.extern test_interrupt_handler
.align 4
.global test_isr
.type test_isr, @function
test_isr:
	call test_interrupt_handler
	iret

.align 4
.extern invalid_opcode_handler
.global invalid_opcode_isr
.type invalid_opcode_isr, @function
invalid_opcode_isr:
	push %ebp
	mov %esp, %ebp

	push (%esp)
	call invalid_opcode_handler

	mov %ebp, %esp
	pop %ebp
	iret

.align 4
.extern bound_range_exceeded_handler
.global bound_range_isr
.type bound_range_isr, @function
bound_range_isr:
	call bound_range_exceeded_handler
	iret

.align 4
.extern general_prot_fault_handler
.global general_prot_fault_isr
.type general_prot_fault_isr, @function
general_prot_fault_isr:
	pop %eax
	call general_prot_fault_handler
	iret

.align 4
.extern double_fault_handler
.global double_fault_isr
.type double_fault_isr, @function
double_fault_isr:
	pop %eax
	call double_fault_handler
	iret

