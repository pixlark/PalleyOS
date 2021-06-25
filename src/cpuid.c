#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <kstdio.h>
#include <cpuid.h>

struct cpuid_struct {
	char vendor_id[13];
	uint32_t ecx_features;
	uint32_t edx_features;
} cpuid = {0};

extern uint32_t loadCpuVendorName(uint32_t* name_buff);
extern uint32_t isCpuidAvailable();

static bool fetchCpuid() {
	uint32_t buff[3];
	loadCpuVendorName(buff);

	int i = 0;
	/* Breaks if I put the second while loop into the for loop :(((( */
	for(int j = 0; j < 2; j++){
		while(buff[j] > 0) {
			cpuid.vendor_id[i] = (char) (buff[j] & 0xff);
			buff[j] >>= 8;
			i++;
		}
	}

	while(buff[2] > 0) {
		cpuid.vendor_id[i] = (char) (buff[2] & 0xff);
		buff[2] >>= 8;
		i++;
	}
	cpuid.vendor_id[12] = 0;
	
	return true;
}

void loadCpuid() {
	uint32_t available = isCpuidAvailable();

	if(available == 0) {
		kprintf("Unable to load CPUID (not available)");
		return;
	}
	
	fetchCpuid();	
	cpuidLoadFeatures((intptr_t)&cpuid.ecx_features, (intptr_t)&cpuid.edx_features);
	
	if(cpuid.edx_features & (1 << 9)) {
		kprintf("Has Built in APIC!\n");
	}else {
		kprintf("No APIC found :(\n");
	}
}

void cpuidPrintVendor() {
	if(fetchCpuid()) {
        kprintf("vendor id: ");
        kprintf(cpuid.vendor_id);
        kprintf("\n");
	} else 
		kprintf("CPUID not available :(\n");
}



