#ifndef PCI_H
#define PCI_H


uint16_t pci_config_read_word (uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);

uint16_t pci_get_vendor_id(uint8_t bus, uint8_t device, uint8_t func);
uint8_t pci_get_header_type(uint8_t bus, uint8_t device, uint8_t func);

uint8_t pci_get_base_class(uint8_t bus, uint8_t device, uint8_t func);
uint8_t pci_get_sub_class(uint8_t bus, uint8_t device, uint8_t func);

uint8_t pci_get_secondary_bus(uint8_t bus, uint8_t device, uint8_t func);

void pci_check_function(uint8_t bus, uint8_t device, uint8_t func);
void pci_check_device(uint8_t bus, uint8_t device);
void pci_check_bus(uint8_t bus);
void pci_check_all_buses(void);

#endif
