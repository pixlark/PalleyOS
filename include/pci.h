#pragma once

#include <stddef.h>
#include <stdint.h>

struct pci_device {
	uint8_t  bus;
	uint8_t  device;
	uint8_t  function;
	uint16_t vendor_id;
	uint16_t device_id;
	uint16_t command;
	uint16_t status;
	uint8_t	 revision_id;
	uint8_t  prog_if;
	uint8_t  sub_class;
	uint8_t  class_code;
	uint8_t	 cache_line_size;
	uint8_t  latency_timer;
   	uint8_t  header_type;
	uint8_t  bist;
};

typedef struct pci_device PCIDevice;

uint16_t pci_config_read_word (uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);

void pci_check_function(uint8_t bus, uint8_t device, uint8_t func);
void pci_check_device(uint8_t bus, uint8_t device);
void pci_check_bus(uint8_t bus);
void pci_check_all_buses(void);

