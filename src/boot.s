.set ALIGN,		1<<0 #Align loaded modules on page boundaries
.set MEMINFO,	1<<1
.set FLAGS, 	ALIGN | MEMINFO
.set MAGIC,		0x1BADB002
.set CHECKSUM, 	-(MAGIC + FLAGS) # Checksum of above, to prove we are multiboot

.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

.section .bss
    
.align 16
stack_bottom:
.skip 16384 # 16 KiB
stack_top:


.section .text
.global _start
.type _start, @function
_start:
	mov $stack_top, %esp
   
	call kernel_main

	cli
1: 	hlt
	jmp 1b

.global enable_paging
.type enable_paging, @function
enable_paging:
	push %ebp
	mov %esp, %ebp

	# Input should be pointer to page_directory
    mov 8(%esp), %eax
	mov %eax, %cr3

	# Enable PSE (4 MiB pages)
	mov %cr4, %eax
	or $0x00000010, %eax 
	mov %eax, %cr4

	# Enable paging
	mov %cr0, %eax
	or $0x80000001, %eax
	mov %eax, %cr0

	mov %ebp, %esp
	pop %ebp
	ret

.size _start, . - _start

.global flush_tlb
.type flush_tlb, @function
flush_tlb:
    movl %cr3, %eax
    movl %eax, %cr3
    ret

# Function that takes in a pointer to the 
# IDTInfo struct and loads the idt 
.global load_idt
.type load_idt, @function
load_idt:
	push %ebp
	mov %esp, %ebp

	mov 8(%ebp), %eax
	lidt (%eax)
	
	mov %ebp, %esp
	pop %ebp
	ret

.extern gp
# setGdt(uint32_t* GDT, uint16_t sizeof(GDT))
.global gdt_flush
.type gdt_flush, @function
gdt_flush:
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

