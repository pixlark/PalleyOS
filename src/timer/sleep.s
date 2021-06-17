.section .data
count_down:	.long 0

.section .text
.global TimerIRQ
.type TimerIRQ, @function
TimerIRQ:
	push %eax
	mov (count_down), %eax
	or %eax, %eax	# Quick way to cmp zero
	jz TimerDone
	mov (count_down), %eax
	dec %eax
	mov %eax, (count_down)
TimerDone:
	mov $0x20, %al
	outb %al, $0x20
	mov $0xa0, %al
	outb %al, $0x20
	pop %eax
	iretl

.global sleep
.type sleep, @function
sleep:
	push %ebp
	mov %esp, %ebp
	push %eax
	mov 8(%ebp), %eax	# eax holds the only argument
	mov %eax, (count_down)
SleepLoop:
	cli	# Don't want to be interrupted for test ;)
	mov (count_down), %eax
	or %eax, %eax
	jz SleepDone
	sti
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	jmp SleepLoop
SleepDone:
	sti
	pop %eax
	mov %ebp, %esp
	pop %ebp
	ret
