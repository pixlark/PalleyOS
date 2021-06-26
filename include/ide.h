#pragma once
/*
   Enumerating the PCI, we find IDE Controllers or ATA Controllers.
   We know they are the correct type of controllers because the
   Class Code is 0x01 and Subclass code is either 0x01(IDE) or 0x05(ATA)

   We can read the Base Address Registers to give us more info.

BAR0 is the start of the I/O ports used by the primary channel.
BAR1 is the start of the I/O ports which control the primary channel.
BAR2 is the start of the I/O ports used by secondary channel.
BAR3 is the start of the I/O ports which control secondary channel.

These are for bus mastering:
BAR4 is the start of 8 I/O ports controls the primary channel's Bus Master IDE.
BAR4 + 8 is the Base of 8 I/O ports controls secondary channel's Bus Master IDE.
*/

#include <stddef.h>
#include <stdint.h>

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

typedef struct IDEDevice IDEDevice;

struct PRD {
    uint32_t address;
    uint16_t byte_count;
    uint16_t reserved;
};

typedef struct PRD PRD;

enum ATAError {
    NoError,
    GeneralError,
    DeviceFault,
    NoDevice,
    DRQError,
    WriteProtected,
    DriveNotFound,
    DeviceNotATA,
    LBAOutOfRange,
};

typedef enum ATAError ATAError;


void ata_test();
void ideInitialize(uint32_t BAR0, uint32_t BAR1, uint32_t BAR2, uint32_t BAR3, uint32_t BAR4);
ATAError ideWriteSectors(uint8_t drive, uint8_t num_sects, uint32_t lba, char * buffer);
ATAError ideReadSectors(uint8_t drive, uint8_t num_sects, uint32_t lba, char* buffer);

void ideIRQHandler();

