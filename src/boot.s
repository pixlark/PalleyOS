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

    ## Enable paging
    #mov $page_directory, %eax
    #mov %eax, %cr3
    #mov %cr0, %eax
    #or  0x80000001, %eax
    #mov %eax, %cr0

    ## Enable PSE (4 MiB pages)
    #mov %cr4, %eax
    #or  %eax, 0x00000010
    #mov %eax, %cr4
    
	call kernel_main

	cli
1: 	hlt
	jmp 1b

.size _start, . - _start

.global flush_tlb
.type flush_tlb, @function
flush_tlb:
    movl %cr3, %eax
    movl %eax, %cr3
    ret
