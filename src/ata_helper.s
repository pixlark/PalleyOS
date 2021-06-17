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

#ata_read_from_port(uint16_t port, uintptr_t buff, uint32_t num_reads)
.global ata_read_from_port
.type ata_read_from_port, @function
ata_read_from_port:
	push %ebp
	mov %esp, %ebp

	mov 16(%ebp), %ecx
	mov 12(%ebp), %edi
	movw 8(%ebp), %dx

.read_loop:
    inw %dx, %ax 
    movw %ax, (%edi)
    add $2, %edi
    dec %ecx
    jnz .read_loop
	#rep insw

	mov %ebp, %esp
	pop %ebp
	ret

#ata_write_from_port(uint16_t port, uintptr_t buff, uint32_t num_writes)
.global ata_write_to_port,
.type ata_write_to_port, @function
ata_write_to_port:
	push %ebp
	mov %esp, %ebp

	mov 16(%ebp), %ecx # num_writes
	mov 12(%ebp), %edi # buffer pointer
	movw 8(%ebp), %dx  # port #

.write_loop:
    movw (%edi), %ax
    outw %ax, %dx
    add $2, %edi
    nop
    nop
    nop
	#rep outsw
    dec %ecx
    jnz .write_loop
    
	mov %ebp, %esp
	pop %ebp
	ret

