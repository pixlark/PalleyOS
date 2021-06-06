# This file handles fetching data from the CPUID

.section .text
# Check is CPUID is supported by attempting to change the "ID" bit (0x0020_0000) in eflags
# this is modifiable only when CPUID is supported
.extern test_print
.global is_CPUID_available
.type is_CPUID_available, @function
is_CPUID_available:
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
.global load_cpu_vendor_name
.type load_cpu_vendor_name, @function
load_cpu_vendor_name:
	push %ebp
	mov %esp, %ebp


	mov $0, %eax
	cpuid

	mov 8(%esp), %edi
	mov %ebx, (%edi)
	mov %edx, 4(%edi)
	mov %ecx, 8(%edi)
		
	mov %ebp, %esp
	pop %ebp
	ret

# Input:
# 	ptr of where to load ecx
# 	ptr of where to load edx
.global load_cpuid_features
.type load_cpuid_features, @function
load_cpuid_features:
	push %ebp
	mov %esp, %ebp
	
	mov $1, %eax
	cpuid

	mov 8(%esp), %ebx
	mov %ecx, (%ebx)
	mov 12(%esp), %ebx
	mov %edx, (%ebx)

	mov %ebp, %esp
	pop %ebp
	ret
