.section .text

# This is called at the beginning of ide_read_buffer
# It would be inline asm, but doing it in c trashes ES and ESP for all of the code
# between begin and end
.global read_buffer_begin
.type read_buffer_begin, @function
read_buffer_begin:
	pushw %es
	movw %ds, %ax
	movw %ax, %es
	ret

.global read_buffer_end
.type read_buffer_end, @function
read_buffer_end:
	popw %es
	ret

.global insl
.type insl, @function
insl:
# parameters are (uint16_t port, uint32_t buffer, uint32_t number_of_reads)
	push %ebp
	mov %esp, %ebp

	movl 8(%esp), %ebx
.loop:
	movw 14(%esp), %dx
	movl 12(%esp), %edi
	insl
	dec %ebx
	jnz .loop

	mov %ebp, %esp
	pop %ebp
	ret
