#pragma once
#include <stddef.h>
#include <stdint.h>
/*
   Enumerating the PCI, we find IDE Controllers or ATA Controllers.
   We know they are the correct type of controllers because the
   Class Code is 0x01 and Subclass code is either 0x01(IDE) or 0x05(ATA)
*/

#define ATA_SECTOR_SIZE 512 // TODO(Anyone): Not actually used by ata.c yet...

struct IDEChannelRegisters {
	uint16_t base; 			// I/O Base	
	uint16_t ctrl;		    // Control Base	
	uint16_t bmide;			// Bus Master IDE
	uint8_t  no_interrupt;	
};

typedef struct IDEChannelRegisters IDEChannelRegisters;

struct IDEDevice {
	uint8_t reserved;			// 0 (Empty) or 1 (This Drive Really Exists)
	uint8_t channel;			// 0 (Primary Channel) or 1 (Secondary Channel)
	uint8_t drive;				// 0 (Master Drive) or 1 (Slave Drive)
	uint16_t type;				// 0:ATA, 1:ATAPI
	uint16_t signature;			// Drive Signature
	uint16_t capabilities;		// Features
	uint32_t command_sets;		// Command Sets Supported
	uint32_t size;				// Size in sectors	
	unsigned char model[41]; 	// Model string
};

typedef struct ide_device IDEDevice;

extern struct IDEDevice ide_devices[4];

struct PRD {
    uint32_t address;
    uint16_t byte_count;
    uint16_t reserved;
};

typedef struct PRD PRD;

void ata_test();
void ide_initialize(uint32_t BAR0,uint32_t BAR1,uint32_t BAR2,uint32_t BAR3,uint32_t BAR4);
uint8_t ideWriteSectors(uint8_t drive, uint8_t num_sects, uint32_t lba, char * buffer);
uint8_t ideReadSectors(uint8_t drive, uint8_t num_sects, uint32_t lba, char* buffer);

