.section .text
# Keyboard Interrupt Handler
.extern keyboard_interrupt_handler

.global keyboard_isr
.type keyboard_isr,@function
keyboard_isr:
	pushal
	cld
	call keyboard_interrupt_handler
	popal
	// Send EOI to PIC (Programmable Interrupt Controller)
	mov $0x20, %al  // Handles IRQ's 0-7
	out %al, $0x20
	mov $0xa0, %al	// Handles IRQ's 8-15
	out %al, $0x20
	iret

# ====== EXCEPTIONS =====

# Divide By Zero (Fault) (0)

.extern div_by_zero_handler
.global div_by_zero_isr
.type div_by_zero_isr, @function
div_by_zero_isr:
	pushal
	cld
	call div_by_zero_handler
	popal
	iret

# Debug (Fault/Trap) (1)

.extern debug_handler
.global debug_isr
.type debug_isr, @function
debug_isr:
	pushal
	cld
	call debug_handler
	popal
	iret
	
# Breakpoint (Trap) (3)

.extern breakpoint_handler
.global breakpoint_isr
.type breakpoint_isr, @function
breakpoint_isr:
	pushal
	cld
	call breakpoint_handler
	popal
	iret

# Overflow (Trap) (4)

.extern overflow_handler
.global overflow_isr
.type overflow_isr, @function
overflow_isr:
	pushal
	cld
	call overflow_handler
	popal
	iret

# Bound Range Exceeded Fault (5)

.extern bound_range_exceeded_handler
.global bound_range_isr
.type bound_range_isr, @function
bound_range_isr:
	pushal
	cld
	call bound_range_exceeded_handler
	popal
	iret

# Invalid Opcode Fault (6)

.extern invalid_opcode_handler
.global invalid_opcode_isr
.type invalid_opcode_isr, @function
invalid_opcode_isr:
	pushal
	cld
	call invalid_opcode_handler
	popal
	iret

# Device Not Available Fault (7) 

.extern device_na_handler
.global device_na_isr
.type device_na_isr, @function
device_na_isr:
	pushal
	cld
	call device_na_handler
	popal
	iret

# Double Fault (Abort) (8)

.extern double_fault_handler
.global double_fault_isr
.type double_fault_isr, @function
double_fault_isr:
	pushal
	cld
	call double_fault_handler
	popal
	iret

# Invalid TSS Fault (10)

.extern invalid_tss_handler
.global invalid_tss_isr
.type invalid_tss_isr, @function
invalid_tss_isr:
	pushal
	cld
	call invalid_tss_handler
	popal

	iret

# Segment Not Present Fault (11)

.extern seg_not_pres_handler
.global seg_not_pres_isr
.type seg_not_pres_isr, @function
seg_not_pres_isr:
	pushal
	cld
	call seg_not_pres_handler
	popal

	iret

# Stack Segment Fault (12)

.extern stack_seg_handler
.global stack_seg_isr
.type stack_seg_isr, @function
stack_seg_isr:
	pushal
	cld
	call stack_seg_handler
	popal

	iret

# General Protection Fault (13)

.extern general_prot_fault_handler
.global general_prot_fault_isr
.type general_prot_fault_isr, @function
general_prot_fault_isr:
	
	pushal
	cld
	call general_prot_fault_handler
	popal
	iret

# Page Fault (14)

.extern page_fault_handler
.global page_fault_isr
.type page_fault_isr, @function
page_fault_isr:
	pushal
	cld
	# Pops err off the stack
	call page_fault_handler
	pop %eax
	popal

	iret

# x87 Floating-Point Exception (Fault) (16)

.extern fpe_handler
.global fpe_isr
.type fpe_isr, @function
fpe_isr:
	pushal
	cld
	call fpe_handler
	popal

	iret

# Alignment Check Fault (17)

.extern align_check_handler
.global align_check_isr
.type align_check_isr, @function
align_check_isr:
	pushal
	cld
	call align_check_handler
	popal

	iret

# Machine Check (18)

.extern machine_check_handler
.global machine_check_isr
.type machine_check_isr, @function
machine_check_isr:
	pushal
	cld
	call machine_check_handler
	popal
	iret

# SIMD Floating Point Exception (Fault) (19)

.extern simd_fpe_handler
.global simd_fpe_isr
.type simd_fpe_isr, @function
simd_fpe_isr:
	pushal
	cld
	call simd_fpe_handler
	popal
	iret

# Virtualization Exception (Fault) (20)

.extern virt_handler
.global virt_isr
.type virt_isr, @function
virt_isr:
	pushal
	cld
	call virt_handler
	popal
	iret
