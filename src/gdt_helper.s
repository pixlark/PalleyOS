.section .text

.extern gp

# setGdt(uint32_t* GDT, uint16_t sizeof(GDT))
.global gdtFlush
.type gdtFlush, @function
gdtFlush:
	lgdt (gp)
	mov $0x10, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	mov %ax, %ss
	ljmp $0x08, $.reload_CS 
.reload_CS:
	ret
