.section .text

.extern gp

# setGdt(uint32_t* GDT, uint16_t sizeof(GDT))
.global gdtFlush
.type gdtFlush, @function
gdtFlush:
lgdt (gp)
ljmp $0x08, $.reload_CS 

.reload_CS:
mov $0x10, %ax
mov %ax, %ds
mov %ax, %es
mov %ax, %fs
mov %ax, %gs
mov %ax, %ss
ret


.global get_gdt_register_value
.type get_gdt_register_value, @function

# Argument: pointer to store value in

get_gdt_register_value:
push %ebp
mov %esp, %ebp

mov 0x8(%ebp), %eax
sgdt (%eax)

mov %ebp, %esp
pop %ebp
ret

.global tssFlush
.type tssFlush, @function
tssFlush:
mov $0x2B, %ax # fifth 8-byte selector. Or'ed with 3 to set RPL (requested privilege level)
ltr %ax # load task register, points to TSS, 5th element in GDT in our case 
# 0 indexed
ret
