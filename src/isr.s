.section .text
# Keyboard Interrupt Handler
.extern keyboardInterruptHandler

.global keyboardIsr
.type keyboardIsr,@function
keyboardIsr:
	pushal
	cld
	call keyboardInterruptHandler
	popal
	// Send EOI to PIC (Programmable Interrupt Controller)
	mov $0x20, %al  // Handles IRQ's 0-7
	out %al, $0x20
	mov $0xa0, %al	// Handles IRQ's 8-15
	out %al, $0x20
	iret

# ====== EXCEPTIONS =====

# Divide By Zero (Fault) (0)

.extern divByZeroHandler
.global divByZeroIsr
.type divByZeroIsr, @function
divByZeroIsr:
	pushal
	cld
	call divByZeroHandler
	popal
	iret

# Debug (Fault/Trap) (1)

.extern debugHandler
.global debugIsr
.type debugIsr, @function
debugIsr:
	pushal
	cld
	call debugHandler
	popal
	iret
	
# Breakpoint (Trap) (3)

.extern breakpointHandler
.global breakpointIsr
.type breakpointIsr, @function
breakpointIsr:
	pushal
	cld
	call breakpointHandler
	popal
	iret

# Overflow (Trap) (4)

.extern overflowHandler
.global overflowIsr
.type overflowIsr, @function
overflowIsr:
	pushal
	cld
	call overflowHandler
	popal
	iret

# Bound Range Exceeded Fault (5)

.extern boundRangeExceededHandler
.global boundRangeIsr
.type boundRangeIsr, @function
boundRangeIsr:
	pushal
	cld
	call boundRangeExceededHandler
	popal
	iret

# Invalid Opcode Fault (6)
.extern invalidOpcodeHandler
.global invalidOpcodeIsr
.type invalidOpcodeIsr, @function
invalidOpcodeIsr:
	pushal
	cld
	call invalidOpcodeHandler
	popal
	iret

# Device Not Available Fault (7) 
.extern deviceNAHandler
.global deviceNAIsr
.type deviceNAIsr, @function
deviceNAIsr:
	pushal
	cld
	call deviceNAHandler
	popal
	iret

# Double Fault (Abort) (8)

.extern doubleFaultHandler
.global doubleFaultIsr
.type doubleFaultIsr, @function
doubleFaultIsr:
	pushal
	cld
	call doubleFaultHandler
	popal
	iret

# Invalid TSS Fault (10)

.extern invalidTSSHandler
.global invalidTSSIsr
.type invalidTSSIsr, @function
invalidTSSIsr:
	pushal
	cld
	call invalidTSSHandler
	popal

	iret

# Segment Not Present Fault (11)

.extern segNotPresHandler
.global segNotPresIsr
.type segNotPresIsr, @function
segNotPresIsr:
	pushal
	cld
	call segNotPresHandler
	popal

	iret

# Stack Segment Fault (12)

.extern stackSegHandler
.global stackSegIsr
.type stackSegIsr, @function
stackSegIsr:
	pushal
	cld
	call stackSegHandler
	popal

	iret

# General Protection Fault (13)

.extern generalProtFaultHandler
.global generalProtFaultIsr
.type generalProtFaultIsr, @function
generalProtFaultIsr:
	
	pushal
	cld
	call generalProtFaultHandler
	popal
	iret

# Page Fault (14)
.extern termWriteInt
.extern pageFaultHandler
.global pageFaultIsr
.type pageFaultIsr, @function
pageFaultIsr:
	pushal
	cld
	call pageFaultHandler
	popal
	add $4, %esp # Get rid of error

	iret

# x87 Floating-Point Exception (Fault) (16)

.extern fpeHandler
.global fpeIsr
.type fpeIsr, @function
fpeIsr:
	pushal
	cld
	call fpeHandler
	popal

	iret

# Alignment Check Fault (17)

.extern align_check_handler
.global alignCheckIsr
.type alignCheckIsr, @function
alignCheckIsr:
	pushal
	cld
	call align_check_handler
	popal

	iret

# Machine Check (18)

.extern machine_check_handler
.global machineCheckIsr
.type machineCheckIsr, @function
machineCheckIsr:
	pushal
	cld
	call machine_check_handler
	popal
	iret

# SIMD Floating Point Exception (Fault) (19)

.extern simdFpeHandler
.global simdFpeIsr
.type simdFpeIsr, @function
simdFpeIsr:
	pushal
	cld
	call simdFpeHandler
	popal
	iret

# Virtualization Exception (Fault) (20)

.extern virtHandler
.global virtIsr
.type virtIsr, @function
virtIsr:
	pushal
	cld
	call virtHandler
	popal
	iret
