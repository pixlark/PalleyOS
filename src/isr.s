.section .text
# Keyboard Interrupt Handler
.extern keyboard_interrupt_handler

.global keyboard_isr
.type keyboard_isr,@function
keyboard_isr:
	call keyboard_interrupt_handler
	iret

# ====== EXCEPTIONS =====

# Divide By Zero (Fault) (0)

.extern div_by_zero_handler
.global div_by_zero_isr
.type div_by_zero_isr, @function
div_by_zero_isr:
	call div_by_zero_handler
	iret

# Debug (Fault/Trap) (1)

.extern debug_handler
.global debug_isr
.type debug_isr, @function
debug_isr:
	call debug_handler
	iret
	
# Breakpoint (Trap) (3)

.extern breakpoint_handler
.global breakpoint_isr
.type breakpoint_isr, @function
breakpoint_isr:
	call breakpoint_handler
	iret

# Overflow (Trap) (4)

.extern overflow_handler
.global overflow_isr
.type overflow_isr, @function
overflow_isr:
	call overflow_handler
	iret

# Bound Range Exceeded Fault (5)

.extern bound_range_exceeded_handler
.global bound_range_isr
.type bound_range_isr, @function
bound_range_isr:
	call bound_range_exceeded_handler
	iret

# Invalid Opcode Fault (6)

.extern invalid_opcode_handler
.global invalid_opcode_isr
.type invalid_opcode_isr, @function
invalid_opcode_isr:
	call invalid_opcode_handler
	iret

# Device Not Available Fault (7) 

.extern device_na_handler
.global device_na_isr
.type device_na_isr, @function
device_na_isr:
	call device_na_handler
	iret

# Double Fault (Abort) (8)

.extern double_fault_handler
.global double_fault_isr
.type double_fault_isr, @function
double_fault_isr:
	pop %eax
	call double_fault_handler
	iret

# Invalid TSS Fault (10)

.extern invalid_tss_handler
.global invalid_tss_isr
.type invalid_tss_isr, @function
invalid_tss_isr:
	push %ebp
	mov %esp, %ebp

	push (%esp)
	call invalid_tss_handler

	mov %ebp, %esp
	pop %ebp
	iret

# Segment Not Present Fault (11)

.extern seg_not_pres_handler
.global seg_not_pres_isr
.type seg_not_pres_isr, @function
seg_not_pres_isr:
	push %ebp
	mov %esp, %ebp

	push (%esp)
	call seg_not_pres_handler

	mov %ebp, %esp
	pop %ebp
	iret

# Stack Segment Fault (12)

.extern stack_seg_handler
.global stack_seg_isr
.type stack_seg_isr, @function
stack_seg_isr:
	push %ebp
	mov %esp, %ebp

	push (%esp)
	call stack_seg_handler

	mov %ebp, %esp
	pop %ebp
	iret

# General Protection Fault (13)

.extern general_prot_fault_handler
.global general_prot_fault_isr
.type general_prot_fault_isr, @function
general_prot_fault_isr:
	push %ebp
	mov %esp, %ebp

	pop %eax
	call general_prot_fault_handler

	mov %ebp, %esp
	pop %ebp
	iret

# Page Fault (14)

.extern page_fault_handler
.global page_fault_isr
.type page_fault_isr, @function
page_fault_isr:
	push %ebp
	mov %esp, %ebp

	pop %eax
	call page_fault_handler

	mov %ebp, %esp
	pop %ebp
	iret

# x87 Floating-Point Exception (Fault) (16)

.extern fpe_handler
.global fpe_isr
.type fpe_isr, @function
fpe_isr:
	call fpe_handler
	iret

# Alignment Check Fault (17)

.extern align_check_handler
.global align_check_isr
.type align_check_isr, @function
align_check_isr:
	push %ebp
	mov %esp, %ebp

	pop %eax
	call align_check_handler

	mov %ebp, %esp
	pop %ebp
	iret

# Machine Check (18)

.extern machine_check_handler
.global machine_check_isr
.type machine_check_isr, @function
machine_check_isr:
	call machine_check_handler
	iret

# SIMD Floating Point Exception (Fault) (19)

.extern simd_fpe_handler
.global simd_fpe_isr
.type simd_fpe_isr, @function
simd_fpe_isr:
	call simd_fpe_handler
	iret

# Virtualization Exception (Fault) (20)

.extern virt_handler
.global virt_isr
.type virt_isr, @function
virt_isr:
	call virt_handler
	iret
