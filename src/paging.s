.section .text
.global load_page_directory
.type load_page_directory, @function
load_page_directory:
	push %ebp
	mov %esp, %ebp
	mov 8(%esp), %eax
	mov %eax, %cr3
	mov %ebp, %esp
	pop %ebp
	ret


.global enable_paging
.type enable_paging, @function
enable_paging:
	push %ebp
	mov %esp, %ebp
	# Enable paging
	mov %cr0, %eax
	or $0x80000001, %eax
	mov %eax, %cr0

	# Enable PSE (4 MiB pages)
	mov %cr4, %eax
	or %eax, $0x00000010
	mov %eax, %cr4

	mov %ebp, %esp
	pop %ebp
	ret


