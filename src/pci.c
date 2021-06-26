#include <stdint.h>
#include <stddef.h>
#include <io.h>
#include <kstdio.h>
#include <pci.h>
#include <ide.h>

#define PCI_CONF_ADDR	0xCF8
#define PCI_CONF_DATA	0xCFC

uint16_t pciConfigReadWord (uint8_t bus, uint8_t device, uint8_t func, uint8_t offset) {
	uint32_t address;
	uint32_t lbus  = (uint32_t)bus;
	uint32_t ldev = (uint32_t)device;
	uint32_t lfunc = (uint32_t)func;
	uint16_t tmp = 0;

	address = (uint32_t)((lbus<<16) | (ldev << 11) | 
			  (lfunc << 8) | (offset & 0xfc) | ((uint32_t) 1 << 31));

	cli();
	outl(PCI_CONF_ADDR, address);
	tmp = (uint16_t)((inl(PCI_CONF_DATA) >> ((offset & 2) * 8)) & 0xffff);
	sti();
	return tmp;
}

PCIDevice pciGetDevice(uint8_t bus, uint8_t device, uint8_t func) {
	PCIDevice ret = {0};	

	ret.bus = bus;
	ret.device = device;
	ret.function = func;

	ret.vendor_id = pciConfigReadWord(bus, device, func, 0x00);
	ret.device_id = pciConfigReadWord(bus, device, func, 0x02);
	ret.command = pciConfigReadWord(bus, device, func, 0x04);
	ret.status = pciConfigReadWord(bus, device, func, 0x06);

	ret.revision_id = pciConfigReadWord(bus, device, func, 0x08) & 0xff;
	ret.prog_if = pciConfigReadWord(bus, device, func, 0x08) >> 8;

	ret.sub_class = pciConfigReadWord(bus, device, func, 0x0a) & 0xff;
	ret.class_code = pciConfigReadWord(bus, device, func, 0x0a) >> 8;
	
	ret.cache_line_size = pciConfigReadWord(bus, device, func, 0x0c) & 0xff;
	ret.latency_timer = pciConfigReadWord(bus, device, func, 0x0c) >> 8;

	ret.header_type = pciConfigReadWord(bus, device, func, 0x0e) & 0xff;
	ret.bist = pciConfigReadWord(bus, device, func, 0x0e) >> 8;

    ret.interrupt_line = pciConfigReadWord(bus, device, func, 0x3c) & 0xff;
    ret.interrupt_pin = pciConfigReadWord(bus, device, func, 0x3c) >> 8;

	return ret;
}

uint32_t pciReadBar(uint8_t bus, uint8_t device, uint8_t func, uint8_t bar_num) {
	uint32_t address;
	uint32_t lbus  = (uint32_t)bus;
	uint32_t ldevice = (uint32_t)device;
	uint32_t lfunc = (uint32_t)func;
	uint16_t tmp = 0;
	address = (uint32_t)((lbus<<16) | (ldevice << 11) | 
			  (lfunc << 8) | ((0x10 + bar_num*4) & 0xfc) | ((uint32_t) 1 << 31));

	cli();
	outl(PCI_CONF_ADDR, address);
	tmp = inl(PCI_CONF_DATA);
	sti();

	return tmp;
}

static void pciWriteBar(uint8_t bus, uint8_t device, uint8_t func, uint8_t bar_num, uint32_t val) {
	uint32_t address;
	uint32_t lbus  = (uint32_t)bus;
	uint32_t ldevice = (uint32_t)device;
	uint32_t lfunc = (uint32_t)func;
	address = (uint32_t)((lbus<<16) | (ldevice << 11) | 
			  (lfunc << 8) | ((0x10 + bar_num*4) & 0xfc) | ((uint32_t) 1 << 31));
	
	cli();
	outl(PCI_CONF_ADDR, address);
	outl(PCI_CONF_DATA, val); 
	sti();
}

static uint32_t getBarMemAmt(PCIDevice pd, uint8_t bar_num) {
	// Read and save bar value
	uint32_t original_bar = pciReadBar(pd.bus, pd.device, pd.function, bar_num);

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
	pciWriteBar(pd.bus, pd.device, pd.function, bar_num, 0xFFFFFFFF);

	// Read back new value
	uint32_t bar_val = pciReadBar(pd.bus, pd.device, pd.function, bar_num);
	if((original_bar & 1) == 0) 	// Memory Space BAR Layout
		bar_val &= 0xFFFFFFF0;
	else  						// I/O Space BAR Layout
		bar_val &= 0xFFFFFFFC;

	// Decode by NOT-ting then adding 1
	uint32_t amt_mem = ~(bar_val) + 1;

	// Write back original bar value
	pciWriteBar(pd.bus, pd.device, pd.function, bar_num, original_bar);

	return amt_mem;
}

static uint8_t pciGetSecondaryBus(uint8_t bus, uint8_t device, uint8_t func){
	return pciConfigReadWord(bus, device, func, 0x18) >> 8;
}

void pciCheckFunction(uint8_t bus, uint8_t device, uint8_t func) { 
	PCIDevice pd = pciGetDevice(bus, device, func);

	if( (pd.class_code == 0x06) && (pd.sub_class == 0x04) ) {
		uint8_t secondary_bus = pciGetSecondaryBus(bus, device, func);
		pciCheckBus(secondary_bus);
	}
}

void pciCheckBus(uint8_t bus) {
	uint8_t device;
	for(device = 0; device < 32; device++)
		pciCheckDevice(bus, device);
}

static void printBar(PCIDevice pd, uint32_t bar, uint8_t bar_num, char* padding){
	uint32_t amt_mem = getBarMemAmt(pd, bar_num);
	if(bar & 1) { // I/O Space BAR Layout
		uint32_t addr = bar & 0XFFFFFFFC;	
		kprintf("%sI/O addr: 0x%x\n", padding, addr);
	}else { // Memory Space BAR Layout
		uint32_t addr = bar & 0xFFFFFFF0;
		kprintf("%sMem. Space addr: 0x%x\n", padding, addr);
	}
		kprintf("%sAmt. Mem. Needed: 0x%x\n", padding, amt_mem);
}

static void printDevice(PCIDevice pd, char* padding){
	kprintf("%sVendorID:      0x%x\n", padding, pd.vendor_id);
	kprintf("%s  DeviceID:    0x%x\n", padding, pd.device_id);
	kprintf("%s  ProgIF:      0x%x\n", padding, pd.prog_if);
	kprintf("%s  Header Type: 0x%x\n", padding, pd.header_type);
	kprintf("%s  Class Code:  0x%x\n", padding, pd.class_code);
	kprintf("%s  Sub Class:   0x%x\n", padding, pd.sub_class);
    kprintf("%s  IRQ Pin:     0x%x\n", padding, pd.interrupt_pin);
    kprintf("%s  IRQ Line:    0x%x\n", padding, pd.interrupt_line);
}

static void handleDistController(PCIDevice pd) {
    uint32_t bars[6];
	bars[0] = pciReadBar(pd.bus, pd.device, pd.function, 0);
	bars[1] = pciReadBar(pd.bus, pd.device, pd.function, 1);
	bars[2] = pciReadBar(pd.bus, pd.device, pd.function, 2);
	bars[3] = pciReadBar(pd.bus, pd.device, pd.function, 3);
	bars[4] = pciReadBar(pd.bus, pd.device, pd.function, 4);
	bars[5] = pciReadBar(pd.bus, pd.device, pd.function, 5);
	ideInitialize(bars);	
}

void pciCheckDevice(uint8_t bus, uint8_t device) {
	uint8_t function = 0;

	PCIDevice pd = pciGetDevice(bus, device, function);

#if DEBUG
	uint16_t vendor_id = pd.vendor_id; 
	if(vendor_id == 0xffff) return;
	uint16_t device_id = pciConfigReadWord(bus, device, 0, 2);

	kprintf("Valid VendorID: 0x%x\n", vendor_id);
	kprintf("    DeviceID: 0x%x\n", device_id);
	kprintf("    Header Type: 0x%x\n", pd.header_type);
	kprintf("    Class Code: 0x%x\n", pd.class_code);
	kprintf("    Sub Class: 0x%x\n", pd.sub_class);
#endif
	

	pciCheckFunction(bus, device, function);
	if(pd.header_type) {
		for(function = 1; function < 8; function++){
			// Slow to call this every time
			PCIDevice tmp_pd = pciGetDevice(bus, device, function);
			if(tmp_pd.vendor_id != 0xffff){ 
//#if DEBUG
				kprintf("    Function %d:\n", function);
				printDevice(tmp_pd, "    ");
//#endif

				pciCheckFunction(bus, device, function);

				if(tmp_pd.class_code == 0x01 && tmp_pd.sub_class == 0x01)
					handleDistController(tmp_pd);

#if DEBUG
				kprintf("Valid VendorID: %x, DeviceId: 0x%x\n", vendor_id, device_id);
#endif
			}
		}
	}

}


void pciCheckAllBuses(void) {
	uint8_t device;

	for(int bus = 0; bus < 256; bus++) {
		for(device = 0; device < 32; device++) {
			pciCheckDevice(bus, device);
		}
	}
}

