#include <stdint.h>
#include <stddef.h>
#include <io.h>
#include <kstdio.h>
#include <pci.h>

struct pci_device {
	uint16_t device_id;
	uint16_t vendor_id;
	uint16_t status;
	uint8_t  class_code;
	uint8_t  sub_class;
	uint8_t  prog_if;
	uint8_t	 revision_id;
   	uint8_t  header_type;
};

uint16_t pci_config_read_word (uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
	uint32_t address;
	uint32_t lbus  = (uint32_t)bus;
	uint32_t lslot = (uint32_t)slot;
	uint32_t lfunc = (uint32_t)func;
	uint16_t tmp = 0;

	address = (uint32_t)((lbus<<16) | (lslot << 11) | 
			  (lfunc << 8) | (offset & 0xfc) | ((uint32_t) 1 << 31));

	disable_interrupts();
	outl(0xcf8, address);
	tmp = (uint16_t)((inl(0xcfc) >> ((offset & 2) * 8)) & 0xffff);
	enable_interrupts();
	return tmp;
}

uint16_t pci_get_vendor_id(uint8_t bus, uint8_t device, uint8_t func) {
	return pci_config_read_word(bus, device, func, 0);
}

uint8_t pci_get_header_type(uint8_t bus, uint8_t device, uint8_t func) {
	return pci_config_read_word(bus, device, func, 0x0c + 0x2) & 0xf;
}

uint8_t pci_get_base_class(uint8_t bus, uint8_t device, uint8_t func) {
	return pci_config_read_word(bus, device, func, 0x08 + 0x2) >> 8;
}

uint8_t pci_get_sub_class(uint8_t bus, uint8_t device, uint8_t func) {
	return pci_config_read_word(bus, device, func, 0x08 + 0x2) ;
}

uint8_t pci_get_secondary_bus(uint8_t bus, uint8_t device, uint8_t func) {
	return (pci_config_read_word(bus, device, func, 0x18) & 0xff00) >> 8;
}

uint32_t pci_read_bar(uint8_t bus, uint8_t device, uint8_t func, uint8_t bar_num) {
	uint32_t address;
	uint32_t lbus  = (uint32_t)bus;
	uint32_t ldevice = (uint32_t)device;
	uint32_t lfunc = (uint32_t)func;
	uint16_t tmp = 0;
	address = (uint32_t)((lbus<<16) | (ldevice << 11) | 
			  (lfunc << 8) | ((0x10 + bar_num*4) & 0xfc) | ((uint32_t) 1 << 31));

	disable_interrupts();
	outl(0xcf8, address);
	tmp = inl(0xcfc);
	enable_interrupts();

	return tmp;
}

static void pci_write_bar(uint8_t bus, uint8_t device, uint8_t func, uint8_t bar_num, uint32_t val) {
	uint32_t address;
	uint32_t lbus  = (uint32_t)bus;
	uint32_t ldevice = (uint32_t)device;
	uint32_t lfunc = (uint32_t)func;
	address = (uint32_t)((lbus<<16) | (ldevice << 11) | 
			  (lfunc << 8) | ((0x10 + bar_num*4) & 0xfc) | ((uint32_t) 1 << 31));
	
	disable_interrupts();
	outl(0xcf8, address);
	outl(0xcfc, val); 
	enable_interrupts();
}

uint32_t decode_bar(uint32_t bar) {
	return bar;
}

void pci_check_function(uint8_t bus, uint8_t device, uint8_t func) { 
	uint8_t base_class;
	uint8_t sub_class;
	uint8_t secondary_bus;

	base_class = pci_get_base_class(bus, device, func);
	sub_class = pci_get_sub_class(bus, device, func);
	if( (base_class == 0x06) && (sub_class == 0x04) ) {
		secondary_bus = pci_get_secondary_bus(bus, device, func);
		pci_check_bus(secondary_bus);
	}
}

void pci_check_bus(uint8_t bus) {
	uint8_t device;

	for(device = 0; device < 32; device++)
		pci_check_device(bus, device);
}


void pci_check_device(uint8_t bus, uint8_t device) {
	uint8_t function = 0;

	uint16_t vendor_id = pci_get_vendor_id(bus, device, 0);
	if(vendor_id == 0xffff) return;
	uint16_t device_id = pci_config_read_word(bus, device, 0, 2);

	kprintf("Valid VendorID: 0x%x\n", vendor_id);
	kprintf("    DeviceID: 0x%x\n", device_id);
	kprintf("    Header Type: 0x%x\n", pci_get_header_type(bus, device, function));
	kprintf("    Base Class: 0x%x\n", pci_get_base_class(bus, device, function));
	kprintf("    Sub Class: ", pci_get_sub_class(bus, device, function));
	pci_write_bar(bus, device, function, 0, 0xfffffff0);
	uint32_t bar0 = pci_read_bar(bus, device, function, 0);
	kprintf("    BAR0: 0x%x\n", decode_bar(bar0));

	pci_check_function(bus, device, function);
	uint16_t header_type = pci_get_header_type(bus, device, function);
	if(header_type) {
		for(function = 1; function < 8; function++){
			if((vendor_id = pci_get_vendor_id(bus, device, function)) != 0xffff){ 
				pci_check_function(bus, device, function);
				device_id = pci_config_read_word(bus, device, 0, 2);

				kprintf("Valid VendorID: %x, DeviceId: 0x%x\n", vendor_id, device_id);
			}
		}
	}

}


void pci_check_all_buses(void) {
//	uint8_t function;
	uint16_t bus;
	uint8_t device;

	for(bus = 0; bus < 256; bus++) {
		for(device = 0; device < 32; device++) {
			pci_check_device(bus, device);
		}
	}
}

