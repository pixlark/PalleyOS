#include <stdint.h>
#include <stddef.h>
#include <io.h>
#include <kstdio.h>
#include <pci.h>
#include <ata.h>

#define PCI_CONF_ADDR	0xCF8
#define PCI_CONF_DATA	0xCFC

uint16_t pci_config_read_word (uint8_t bus, uint8_t device, uint8_t func, uint8_t offset) {
	uint32_t address;
	uint32_t lbus  = (uint32_t)bus;
	uint32_t ldev = (uint32_t)device;
	uint32_t lfunc = (uint32_t)func;
	uint16_t tmp = 0;

	address = (uint32_t)((lbus<<16) | (ldev << 11) | 
			  (lfunc << 8) | (offset & 0xfc) | ((uint32_t) 1 << 31));

	disable_interrupts();
	outl(PCI_CONF_ADDR, address);
	tmp = (uint16_t)((inl(PCI_CONF_DATA) >> ((offset & 2) * 8)) & 0xffff);
	enable_interrupts();
	return tmp;
}

PCIDevice pci_get_device(uint8_t bus, uint8_t device, uint8_t func) {
	PCIDevice ret = {0};	

	ret.bus = bus;
	ret.device = device;
	ret.function = func;

	ret.vendor_id = pci_config_read_word(bus, device, func, 0x00);
	ret.device_id = pci_config_read_word(bus, device, func, 0x02);
	ret.command = pci_config_read_word(bus, device, func, 0x04);
	ret.status = pci_config_read_word(bus, device, func, 0x06);

	ret.revision_id = pci_config_read_word(bus, device, func, 0x08) & 0xff;
	ret.prog_if = pci_config_read_word(bus, device, func, 0x08) >> 8;

	ret.sub_class = pci_config_read_word(bus, device, func, 0x0a) & 0xff;
	ret.class_code = pci_config_read_word(bus, device, func, 0x0a) >> 8;
	
	ret.cache_line_size = pci_config_read_word(bus, device, func, 0x0c) & 0xff;
	ret.latency_timer = pci_config_read_word(bus, device, func, 0x0c) >> 8;

	ret.header_type = pci_config_read_word(bus, device, func, 0x0e) & 0xff;
	ret.bist = pci_config_read_word(bus, device, func, 0x0e) >> 8;

	return ret;
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
	outl(PCI_CONF_ADDR, address);
	tmp = inl(PCI_CONF_DATA);
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
	outl(PCI_CONF_ADDR, address);
	outl(PCI_CONF_DATA, val); 
	enable_interrupts();
}

static uint32_t get_bar_mem_amt(PCIDevice pd, uint8_t bar_num) {
	// Read and save bar value
	uint32_t original_bar = pci_read_bar(pd.bus, pd.device, pd.function, bar_num);

	if((original_bar & 1) == 0) {
		uint8_t type = (original_bar & 5) >> 1;
		if(type == 0x00)
			kprintf("BAR %d is 16-bit addressable\n", bar_num);	
		else if(type == 0x01)
			kprintf("BAR %d is 32-bit addressable\n", bar_num);	
		else if(type == 0x02)
			kprintf("BAR %d is 64-bit addressable\n", bar_num);	
	}else {
		kprintf("BAR %d is in I/O Space\n", bar_num);
	}

	// Write all ones to bar
	pci_write_bar(pd.bus, pd.device, pd.function, bar_num, 0xFFFFFFFF);

	// Read back new value
	uint32_t bar_val = pci_read_bar(pd.bus, pd.device, pd.function, bar_num);
	if((original_bar & 1) == 0) 	// Memory Space BAR Layout
		bar_val &= 0xFFFFFFF0;
	else  						// I/O Space BAR Layout
		bar_val &= 0xFFFFFFFC;

	// Decode by NOT-ting then adding 1
	uint32_t amt_mem = ~(bar_val) + 1;

	// Write back original bar value
	pci_write_bar(pd.bus, pd.device, pd.function, bar_num, original_bar);

	return amt_mem;
}

static uint8_t pci_get_secondary_bus(uint8_t bus, uint8_t device, uint8_t func){
	return pci_config_read_word(bus, device, func, 0x18) >> 8;
}

void pci_check_function(uint8_t bus, uint8_t device, uint8_t func) { 
	PCIDevice pd = pci_get_device(bus, device, func);

	if( (pd.class_code == 0x06) && (pd.sub_class == 0x04) ) {
		uint8_t secondary_bus = pci_get_secondary_bus(bus, device, func);
		pci_check_bus(secondary_bus);
	}
}

void pci_check_bus(uint8_t bus) {
	uint8_t device;
	for(device = 0; device < 32; device++)
		pci_check_device(bus, device);
}

static void print_bar(PCIDevice pd, uint32_t bar, uint8_t bar_num, char* padding){
	uint32_t amt_mem = get_bar_mem_amt(pd, bar_num);
	if(bar & 1) { // I/O Space BAR Layout
		uint32_t addr = bar & 0XFFFFFFFC;	
		kprintf("%sI/O addr: 0x%x\n", padding, addr);
	}else { // Memory Space BAR Layout
		uint32_t addr = bar & 0xFFFFFFF0;
		kprintf("%sMem. Space addr: 0x%x\n", padding, addr);
	}
		kprintf("%sAmt. Mem. Needed: 0x%x\n", padding, amt_mem);
}

static void print_device(PCIDevice pd, char* padding){
	kprintf("%sVendorID:      0x%x\n", padding, pd.vendor_id);
	kprintf("%s  DeviceID:    0x%x\n", padding, pd.device_id);
	kprintf("%s  ProgIF:      0x%x\n", padding, pd.prog_if);
	kprintf("%s  Header Type: 0x%x\n", padding, pd.header_type);
	kprintf("%s  Class Code:  0x%x\n", padding, pd.class_code);
	kprintf("%s  Sub Class:   0x%x\n", padding, pd.sub_class);

	/*
	for(int i = 0; i <= 5; i++){	
		uint32_t bar = pci_read_bar(pd.bus, pd.device, pd.function, i);
		if(bar == 0) continue;
		int pad_size = sizeof(padding)/sizeof(char) + 3;
		char pad[pad_size];		
		print_bar(pd, bar, i, padding);
	}
	*/
}

static void handle_disk_controller(PCIDevice pd) {
	uint32_t bar0 = pci_read_bar(pd.bus, pd.device, pd.function, 0);
	uint32_t bar1 = pci_read_bar(pd.bus, pd.device, pd.function, 1);
	uint32_t bar2 = pci_read_bar(pd.bus, pd.device, pd.function, 2);
	uint32_t bar3 = pci_read_bar(pd.bus, pd.device, pd.function, 3);
	uint32_t bar4 = pci_read_bar(pd.bus, pd.device, pd.function, 4);
	ide_initialize(bar0, bar1, bar2, bar3, bar4);	
}

void pci_check_device(uint8_t bus, uint8_t device) {
	uint8_t function = 0;

	PCIDevice pd = pci_get_device(bus, device, function);

#if DEBUG
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
#endif
	

	pci_check_function(bus, device, function);
	if(pd.header_type) {
		for(function = 1; function < 8; function++){
			// Slow to call this every time
			PCIDevice tmp_pd = pci_get_device(bus, device, function);
			if(tmp_pd.vendor_id != 0xffff){ 
//				kprintf("    Function %d:\n", function);
				pci_check_function(bus, device, function);
//				print_device(tmp_pd, "    ");

				if(tmp_pd.class_code == 0x01 & tmp_pd.sub_class == 0x01)
					handle_disk_controller(pd);

#if DEBUG
				kprintf("Valid VendorID: %x, DeviceId: 0x%x\n", vendor_id, device_id);
#endif
			}
		}
	}

}


void pci_check_all_buses(void) {
	uint8_t device;

	for(int bus = 0; bus < 256; bus++) {
		for(device = 0; device < 32; device++) {
			pci_check_device(bus, device);
		}
	}
}

