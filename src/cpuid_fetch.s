# This file handles fetching data from the CPUID

.section .text
# Check is CPUID is supported by attempting to change the "ID" bit (0x0020_0000) in eflags
# this is modifiable only when CPUID is supported
.extern test_print
.global isCpuidAvailable
.type isCpuidAvailable, @function
isCpuidAvailable:
	pushfl					# Save EFLAGS
	pushfl					# Store EFLAGS
	xorl $0x00200000, (%esp)	# Invert the ID bit in stored EFLAGS
	popfl					# Load stored EFLAGS (ID may or may not be changed)
	pushfl					# Store EFLAGS again
	pop %eax				# eax = modified EFLAGS (ID may or may not be changed)
	xor (%esp), %eax		# eax = whichever bits were changed
	popfl					# Restore original EFLAGS
	and $0x00200000, %eax	# eax = zero if ID bit can't be changes, else non-zero
	ret


# Input: 
# 	pointer to memory location to store 12 character vendor id on success
# Output:
#	int which is -1 on failure and 0 on success
.global loadCpuVendorName
.type loadCpuVendorName, @function
loadCpuVendorName:
	push %ebp
	mov %esp, %ebp

	push %edi
	push %ebx

	mov $0, %eax
	cpuid

	mov 8(%ebp), %edi
	mov %ebx, (%edi)
	mov %edx, 4(%edi)
	mov %ecx, 8(%edi)

	pop %edi
	pop %ebx
		
	mov %ebp, %esp
	pop %ebp
	ret

# Input:
# 	ptr of where to load ecx
# 	ptr of where to load edx
.global cpuidLoadFeatures
.type cpuidLoadFeatures, @function
cpuidLoadFeatures:
	push %ebp
	mov %esp, %ebp

	push %ebx

	mov $1, %eax
	cpuid

	mov 8(%ebp), %ebx
	mov %ecx, (%ebx)
	mov 12(%ebp), %ebx
	mov %edx, (%ebx)

	pop %ebx

	mov %ebp, %esp
	pop %ebp
	ret
