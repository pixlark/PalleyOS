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
	//ideInitialize(bars);	
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

