/*
	The Root System Description Pointer is a data structure used 
	in the Advanced Configuration and Power Interface (ACPI) 
	programming interface.

	Its main use is for locating the Root System Description Table (RSDT)
	which contains pointers to other System Descriptor Tables such as
	the Fixed ACPI Description Table (FADT) which contains information to enable the ACPI

 */
#include <aspci.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

static bool eval_rsdp_checksum(struct RSDPDescriptor* rsdp) {
	uint8_t sum = 0;
	// The sum of all bytes in the rsdp structure should = 0
	for(size_t i = 0; i < sizeof(*rsdp); i++){
		sum += ((char*)rsdp)[i];
	}
	return sum == 0;
}

static struct RSDPDescriptor* get_rsdp(){

	// RSDP will be located in the main BIOS memory below 1MB
	// (0x000E0000 to 0x000FFFFF) on a 16-byte boundary
	// The first 8 bytes of a valid RSDP will be equal to "RSD PTR " 
	// **note the extra space in the 8th byte
	for(char* loc = (char*)0x000E0000; loc <= (char*)0x000FFFFF; loc += 16){
		// strcmp for "RSD PTR ", if success, we have found it!
		if(memcmp("RSD PTR ", (char*)loc, 8) == 0){
			if(!eval_rsdp_checksum((struct RSDPDescriptor*)loc)) continue;
			return (struct RSDPDescriptor*)loc;
		}
	}
	return NULL;
}