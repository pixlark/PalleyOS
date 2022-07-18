.section .text

# This is called at the beginning of ideReadBuffer
# It would be inline asm, but doing it in c trashes ES and ESP for all of the code
# between begin and end
.global readBufferBegin
.type readBufferBegin, @function
readBufferBegin:
	pushw %es
	movw %ds, %ax
	movw %ax, %es
	ret

.global readBufferEnd
.type readBufferEnd, @function
readBufferEnd:
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

#ataReadFromPort(uint16_t port, uintptr_t buff, uint32_t num_reads)
.global ataReadFromPort
.type ataReadFromPort, @function
ataReadFromPort:
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
.global ataWriteToPort,
.type ataWriteToPort, @function
ataWriteToPort:
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



#.extern ideIRQHandler
.global ideIRQISR
.type ideIRQISR, @function
ideIRQISR:
	pushal
	cld
#call ideIRQHandler
	popal
	iret
