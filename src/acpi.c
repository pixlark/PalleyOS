/*
	The ACPI (Advanced Configuration and Power Interface) is a Power Management
	and configuration standard for the PC. It allows the OS (us) to 
	control the amount of power each device is given (with this
	we can put certain devices on standby and power-off).
	It also is used to control and/or check thermal zones (temp sensors,
	fans speeds, etc), battery levels, PCI IRQ routing, CPUs, NUMA Domains
	and other stuff.

	Info about ACPI is stored in BIOS's memory (if supported)

	2 Parts to the ACPI.

	1st is used by the OS for configuration during boot.
	2nd consists of AML Code and the ACPI SMM (System Management Mode) code.
		This is the run time ACPI Environment.

	To being using ACPI, we (the OS) must look for the RSDP (Root System
	Description Pointer).

	If Found, it contains a pointer to the RSDT (Root System Description Pointer)
	and for newer version (ACPI 2.0 and later) there is an additional
	XSDT (eXtended System Description Table). Both RSDT and XSDT contain
	pointers to other tables. XSDT contains 64-bit pointers, so it is not
	important to us.
	
	As for the runtime part, the main thing we need to look for is the FADT
	(Fixed ACPI Description Table). This contains info needed to enable the
	ACPI.

	Since we are doing stuff the hard (fun) way, we have to create our own
	ACPI table reader and AML Interpreter.

	** In order to enable the ACPI we need RSDP -> RSDT -> FADT **
 */

#include <acpi.h>
#include <rsdp.h>

static struct RSDPDescriptor* rsdp = NULL;
static struct RSDT* rsdt = NULL;

bool acpi_eval_header_checksum(struct ACPISDTHeader* header) {
	unsigned char sum = 0;

	for(int i = 0; i < header->length; i++)
		sum += ((char*) header)[i];

	return sum == 0;
}

void acpi_testing() {
	struct RSDPDescriptor* rsdp = get_rsdp();	
	if(rsdp == NULL){
		kprintf("Unable to locate RSDP :(\n");
		return;
	}

	struct RSDT* rsdt = (struct RSDT*)rsdp->rsdt_address;
	
}

