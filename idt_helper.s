.global get_idt_register_value
.type get_idt_register_value, @function
# Argument: pointer to store value in
get_idt_register_value:
push %ebp
mov %esp, %ebp

mov 0x8(%ebp), %eax
sidt (%eax)

mov %ebp, %espi
pop %ebp
ret