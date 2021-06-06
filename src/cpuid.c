#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <tio.h>
#include <cpuid.h>

struct cpuid_struct {
	char vendor_id[13];
	uint32_t ecx_features;
	uint32_t edx_features;
} cpuid = {0};

extern uint32_t load_cpu_vendor_name(uint32_t* name_buff);
extern uint32_t is_CPUID_available();


bool fetch_cpuid() {

	uint32_t buff[3];
	load_cpu_vendor_name(buff);


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
	

	return true;
}

void load_cpuid() {
	uint32_t available = is_CPUID_available();

	if(available == 0) {
		term_write("Unable to load CPUID (not available)");
		return;
	}
	
	fetch_cpuid();	
	load_cpuid_features((intptr_t)&cpuid.ecx_features, (intptr_t)&cpuid.edx_features);
	
	if(cpuid.edx_features & (1 << 9)) {
		term_write("Has Built in APIC!\n");
	}else {
		term_write("No APIC found :(\n");
	}
}


void print_cpuid_vendor() {
	if(fetch_cpuid()) {
	term_write("vendor id: ");
	term_write(cpuid.vendor_id);
	term_write("\n");
	} else {
		term_write("CPUID not available :(\n");
	}
}



