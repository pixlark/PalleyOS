.extern keyboard_interrupt_handler
.align 4
.global keyboard_isr
.type keyboard_isr,@function
keyboard_isr:
	pushal
	cld
	call keyboard_interrupt_handler
	popal
	iret
