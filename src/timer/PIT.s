# The PIT (Programable Interval Timer) allows the programmer
# to set regularly intervalled interrupts (used for timing things, typically)
# Once the countdown hits zero, it can either reload the value or stay at zero

# PIT runs roughly at 1.193182 MHz
# Using the reload value, we can divide the frequency by any 16-bit value
# giving us a slower frequency

.section .bss
# To keep track of ms, we are using fixed point 32.32 floating number
# i.e. 32 bits for before decimal, 32 bits for after decimal
# e.g. The number 0x0000_0001.8000_0000 is 1.5 decimal
.global system_timer_fraction_ms
system_timer_fraction_ms: 	.long 0 # fraction of ms since timer initialized
.global system_timer_whole_ms
system_timer_whole_ms: 		.long 0 # whole ms since timer initialized
.global IRQ0_fraction_ms
IRQ0_fraction_ms: 			.long 0 # fraction of ms between IRQs
.global IRQ0_ms
IRQ0_ms:					.long 0 # whole ms between IRQs
.global PIT_reload_value
PIT_reload_value:			.word 0 # Current PIT value
.global IRQ0_frequency
IRQ0_frequency:				.long 0 # Actual frequency used

.section .text
.extern TimerIRQ
.global IRQ0_handler
.type IRQ0_handler, @function
# The PIT raises an interrupt 0
IRQ0_handler:
	push %eax
	push %ebx

	mov (IRQ0_fraction_ms), %eax	
	mov (IRQ0_ms), %ebx					# eax.ebx = amount of timer between IRQs
	add (system_timer_fraction_ms), %eax 
	adc (system_timer_whole_ms), %ebx

	call TimerIRQ
	
	mov $0x20, %al
	outb %al, $0x20		# Send EOI (End of Interrupt) to the PIC

	pop %ebx
	pop %eax
	iretl				# 32-bit interrupt return

#Input
# 32-bit long on stack, desired frequency
.global init_PIT
.type init_PIT, @function
init_PIT:
	push %ebp
	mov %esp, %ebp

	push %ebx
	push %edx

	mov 8(%ebp), %ebx	# ebx = requested frequency
	
	# Check to make sure the PIT can support the requested frequency
	movl $0x10000, %eax	# eax = reload value for slowest possible freq.
	cmp $18, %ebx 		# too slow?
	jbe .gotReloadValue	# yes, use the slowest possible frequency
						# jbe = jump below equal, unsigned

	movl $0x1, %eax		# eax = reload value for fastest possible
	cmp $1193181, %ebx	# too fast?
	jae .gotReloadValue# jump above equal, like jge but unsigned

	# Calculate the reload value
	movl $3579545, %eax
	movl $0, %edx
	div %ebx			# eax = 1193181 / desired freq., edx = remainder
						# i.e. eax = reload value
	# round up or down to be improve accurate
	cmp $1789772, %edx   # Is the remainder more than half
	jb .l1				# no, round down
	inc %eax			# yes, round up
.l1:
	mov $3, %ebx
	mov $0, %edx
	div %ebx
	cmp $1, %edx
	jb .l2
	inc %eax
.l2:


.gotReloadValue:
	push %eax 					# store reload value for later
	movw %ax, (PIT_reload_value)	# also store it here
	mov %eax, %ebx
	
	mov $3579545, %eax
	mov $0, %edx
	div %ebx
	cmp $1789772, %edx
	jb .l3
	inc %eax
.l3:
	mov $3, %ebx
	mov $0, %edx
	div %ebx
	cmp $1, %edx
	jb .l4
	inc %eax
.l4:
	mov %eax, (IRQ0_frequency)

	# Calculate the amount of time between IRQs in 32.32 fixed point
	# Note: basic formula:
	# 	time in ms = reload_value / 1193181 * 1000
	
	pop %ebx
	mov $0xDBB3A062, %eax	
	mul %ebx				
	shrd $10, %edx, %eax		
	shr $10, %edx

	mov %edx, (IRQ0_ms)			
	mov %eax, (IRQ0_fraction_ms)

	# Program the PIT Channel
	pushfl
	cli

	mov $0b00110100, %al
	outb %al, $0x43

	mov (PIT_reload_value), %ax
	outb %al, $0x40
	mov %ah, %al
	outb %al, $0x40
	sti

	popfl

	pop %edx
	pop %ebx

	mov %ebp, %esp
	pop %ebp
	ret
