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
.global kernel_stack_top
.global kernel_stack_bottom
kernel_stack_bottom:
.skip 1024*16 # 16 KiB 
kernel_stack_top:

.section .text
.global _start
.type _start, @function
_start:
mov $kernel_stack_top, %esp

push %eax
push %ebx

call kernelMain

jmp .

.global enablePaging
.type enablePaging, @function
enablePaging:
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

mov %cr3, %eax

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

.global getFaultAddress
.type getFaultAddress, @function
getFaultAddress:
mov %cr2, %eax
ret

