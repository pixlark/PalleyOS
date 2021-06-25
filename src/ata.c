/*
 * The purpose of this file is to interface with ATA and ATAPI devices
 * ATA is common for hard drives and
 * ATAPI is common for optical drives
 */

#include <stdint.h>
#include <stddef.h>
#include <kstdio.h>
#include <kstdlib.h>
#include <io.h>
#include <ata.h>
#include <timer.h>

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

#define PBUS 	0x170	// Primary Bus I/O
#define PBUSC 	0x3f6	// Primary Bus Control

#define SBUS	0x170
#define SBUSC	0x376

#define IDENTIFY_SUCCESS		0
#define IDENTIFY_NO_DRIVE		1
#define IDENTIFY_NOT_ATA		2
#define IDENTIFY_BUS_ERR   		3
#define IDENTIFY_DRIVE_ATAPI 	4
#define IDENTIFY_DRIVE_SATA 	5
#define IDENTIFY_DRIVE_ABORT 	6
#define IDENTIFY_INVALID_PARAM  7

#define ATA_ERR	1
#define ATA_DRQ 3

/* ===== Status Register ===== */
#define ATA_SR_BSY     0x80    // Busy
#define ATA_SR_DRDY    0x40    // Drive ready
#define ATA_SR_DF      0x20    // Drive write fault
#define ATA_SR_DSC     0x10    // Drive seek complete
#define ATA_SR_DRQ     0x08    // Data request ready
#define ATA_SR_CORR    0x04    // Corrected data
#define ATA_SR_IDX     0x02    // Index
#define ATA_SR_ERR     0x01    // Error

/* ===== Errors ===== */
#define ATA_ER_BBK      0x80    // Bad block
#define ATA_ER_UNC      0x40    // Uncorrectable data
#define ATA_ER_MC       0x20    // Media changed
#define ATA_ER_IDNF     0x10    // ID mark not found
#define ATA_ER_MCR      0x08    // Media change request
#define ATA_ER_ABRT     0x04    // Command aborted
#define ATA_ER_TK0NF    0x02    // Track 0 not found
#define ATA_ER_AMNF     0x01    // No address mark

/* ===== Commands ===== */
#define ATA_CMD_READ_PIO          0x20
#define ATA_CMD_READ_PIO_EXT      0x24
#define ATA_CMD_READ_DMA          0xC8
#define ATA_CMD_READ_DMA_EXT      0x25
#define ATA_CMD_WRITE_PIO         0x30
#define ATA_CMD_WRITE_PIO_EXT     0x34
#define ATA_CMD_WRITE_DMA         0xCA
#define ATA_CMD_WRITE_DMA_EXT     0x35
#define ATA_CMD_CACHE_FLUSH       0xE7
#define ATA_CMD_CACHE_FLUSH_EXT   0xEA
#define ATA_CMD_PACKET            0xA0
#define ATA_CMD_IDENTIFY_PACKET   0xA1
#define ATA_CMD_IDENTIFY          0xEC

#define ATAPI_CMD_READ       0xA8
#define ATAPI_CMD_EJECT      0x1B

#define ATA_IDENT_DEVICETYPE   0
#define ATA_IDENT_CYLINDERS    2
#define ATA_IDENT_HEADS        6
#define ATA_IDENT_SECTORS      12
#define ATA_IDENT_SERIAL       20
#define ATA_IDENT_MODEL        54
#define ATA_IDENT_CAPABILITIES 98
#define ATA_IDENT_FIELDVALID   106
#define ATA_IDENT_MAX_LBA      120
#define ATA_IDENT_COMMANDSETS  164
#define ATA_IDENT_MAX_LBA_EXT  200

/* ===== Interface Types ===== */
#define IDE_ATA        0x00
#define IDE_ATAPI      0x01
 
#define ATA_MASTER     0x00
#define ATA_SLAVE      0x01

/* ===== Register Offsets ===== */
#define ATA_REG_DATA       0x00
#define ATA_REG_ERROR      0x01
#define ATA_REG_FEATURES   0x01
#define ATA_REG_SECCOUNT0  0x02
#define ATA_REG_LBA0       0x03
#define ATA_REG_LBA1       0x04
#define ATA_REG_LBA2       0x05
#define ATA_REG_HDDEVSEL   0x06
#define ATA_REG_COMMAND    0x07
#define ATA_REG_STATUS     0x07
#define ATA_REG_SECCOUNT1  0x08
#define ATA_REG_LBA3       0x09
#define ATA_REG_LBA4       0x0A
#define ATA_REG_LBA5       0x0B
#define ATA_REG_CONTROL    0x0C
#define ATA_REG_ALTSTATUS  0x0C
#define ATA_REG_DEVADDRESS 0x0D

/* ===== Channels / Directions ===== */
// Channels:
#define      ATA_PRIMARY      0x00
#define      ATA_SECONDARY    0x01
 
// Directions:
#define      ATA_READ      0x00
#define      ATA_WRITE     0x01

unsigned char ide_buf[2048] = {0};
static uint8_t ide_irq_invoked = 0;
static uint8_t atapi_packet[12] = {0xA8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
struct IDEDevice ide_devices[4];
struct IDEChannelRegisters channels[2];

static PRD prdt[1] __attribute__((aligned (64*1024)));

void ideWrite(uint8_t channel, uint8_t reg, uint8_t data) {
   if (reg > 0x07 && reg < 0x0C)
      ideWrite(channel, ATA_REG_CONTROL, 0x80 | channels[channel].no_interrupt);
   if (reg < 0x08)
      outb(channels[channel].base  + reg - 0x00, data);
   else if (reg < 0x0C)
      outb(channels[channel].base  + reg - 0x06, data);
   else if (reg < 0x0E)
      outb(channels[channel].ctrl  + reg - 0x0A, data);
   else if (reg < 0x16)
      outb(channels[channel].bmide + reg - 0x0E, data);
   if (reg > 0x07 && reg < 0x0C)
      ideWrite(channel, ATA_REG_CONTROL, channels[channel].no_interrupt);
}

uint8_t ideRead(uint8_t channel, uint8_t reg) {
	uint8_t result = 0;
	if(reg > 0x07 && reg < 0x0c)
		ideWrite(channel, ATA_REG_CONTROL, 0x80 | channels[channel].no_interrupt);
	else if(reg < 0x08)
		result = inb(channels[channel].base + reg - 0x00);
	else if(reg < 0x0C)
		result = inb(channels[channel].base + reg - 0x06);
	else if(reg < 0x0E)
		result = inb(channels[channel].base + reg - 0x0A);
	else if(reg < 0x16)
		result = inb(channels[channel].base + reg - 0x0E);
	if(reg > 0x07 && reg < 0x0C)
		ideWrite(channel, ATA_REG_CONTROL, channels[channel].no_interrupt);
	return result;
}

uint8_t ide_polling (uint8_t channel, uint32_t advanced_check) {
	// 1. Delay 400 nanosecond for BSY to be set
	for(int i = 0; i < 4; i++)
		ideRead(channel, ATA_REG_ALTSTATUS); // This wastes 100ns

	// 2. Wait for BSY to be cleared
	while(ideRead(channel, ATA_REG_STATUS) & ATA_SR_BSY);

	if (advanced_check) {
		uint8_t state = ideRead(channel, ATA_REG_STATUS); 

		// 3. Check for errors
		if (state & ATA_SR_ERR) {
			return 2; // Error
		}

		// 4. Check if Device Fault
		if (state & ATA_SR_DF)
			return 1; // Device Fault

		// 5. Check if DRQ (Data Request Ready)
		if ((state & ATA_SR_DRQ) == 0)
			return 3; // DRQ should be set
	}

	return 0;
}


uint8_t ide_print_error (uint8_t drive, uint8_t err) {
 if (err == 0)
      return err;
 
   kprintf("IDE:");
   if (err == 1) { kprintf("- Device Fault\n    "); err = 19; }
   else if (err == 2) {
      unsigned char st = ideRead(ide_devices[drive].channel, ATA_REG_ERROR);
      if (st & ATA_ER_AMNF)  { kprintf("- No Address Mark Found\n    ");   		err = 7;  }
      if (st & ATA_ER_TK0NF) { kprintf("- No Media or Media Error\n    ");   	err = 3;  }
      if (st & ATA_ER_ABRT)  { kprintf("- Command Aborted\n    ");      		err = 20; }
      if (st & ATA_ER_MCR)   { kprintf("- No Media or Media Error\n    ");   	err = 3;  }
      if (st & ATA_ER_IDNF)  { kprintf("- ID mark not Found\n    ");      		err = 21; }
      if (st & ATA_ER_MC)    { kprintf("- No Media or Media Error\n     ");   	err = 3;  }
      if (st & ATA_ER_UNC)   { kprintf("- Uncorrectable Data Error\n    ");   	err = 22; }
      if (st & ATA_ER_BBK)   { kprintf("- Bad Sectors\n    ");       			err = 13; }
   } else if (err == 3)  { kprintf("- Reads Nothing\n    "); 	err = 23; }
     else if (err == 4)  { kprintf("- Write Protected\n    "); 	err = 8;  }

   kprintf("- [%s %s] %s\n",
      (const char *[]){"Primary", "Secondary"}[ide_devices[drive].channel], // Use the channel as an index into the array
      (const char *[]){"Master", "Slave"}[ide_devices[drive].drive], 		// Same as above, using the drive
      ide_devices[drive].model);
 
   return err;
}

void read_insl(uint16_t port, uint32_t* buffer, uint32_t num_times){
	while(num_times--)	{
		*buffer = inl(port);
		buffer++;
	}
}

extern void readBufferBegin(void);
extern void insl (uint16_t port, uint32_t buffer, uint32_t num_times);
extern void readBufferEnd(void);
void ideReadBuffer (uint8_t channel, uint8_t reg, uint32_t* buffer,
		uint32_t num_quads) {
	if (reg > 0x07 && reg < 0x0C)
		ideWrite(channel, ATA_REG_CONTROL, 0x80 | channels[channel].no_interrupt);

	if (reg < 0x08)
		read_insl(channels[channel].base + reg - 0x00, buffer, num_quads);
	else if (reg < 0x0C)
		read_insl(channels[channel].base + reg - 0x06, buffer, num_quads);
	else if (reg < 0x0E)
		read_insl(channels[channel].ctrl + reg - 0x0A, buffer, num_quads);
	else if (reg < 0x16)
		read_insl(channels[channel].bmide + reg - 0x0E, buffer, num_quads);

	if (reg > 0x07 && reg < 0x0C)
		ideWrite(channel, ATA_REG_CONTROL, channels[channel].no_interrupt);
}


void ide_initialize (uint32_t BAR0, uint32_t BAR1, uint32_t BAR2, uint32_t BAR3, uint32_t BAR4){
	// 1. Detect I/O Ports which interface with IDE Controller
	channels[ATA_PRIMARY  ].base  = (BAR0 & 0xFFFFFFFC) + 0x1F0 * (!BAR0);
	channels[ATA_PRIMARY  ].ctrl  = (BAR1 & 0xFFFFFFFC) + 0x3F6 * (!BAR1);
	channels[ATA_SECONDARY].base  = (BAR2 & 0xFFFFFFFC) + 0x170 * (!BAR2);
	channels[ATA_SECONDARY].ctrl  = (BAR3 & 0xFFFFFFFC) + 0x376 * (!BAR3);
	channels[ATA_PRIMARY  ].bmide = (BAR4 & 0xFFFFFFFC) + 0; // Bus Master IDE
	channels[ATA_SECONDARY].bmide = (BAR4 & 0xFFFFFFFC) + 8; // Bus Master IDE
    kprintf("bmide_prim 0x%x\n", channels[ATA_PRIMARY  ].bmide);
    kprintf("bmide_secondary 0x%x\n", channels[ATA_SECONDARY  ].bmide);

	// 2. Disable IRQs in both channels
	ideWrite(ATA_PRIMARY  , ATA_REG_CONTROL, 2); // setting nIEN (no_interrupt) in Control Port
	ideWrite(ATA_SECONDARY, ATA_REG_CONTROL, 2); // setting nIEN (no_interrupt) in Control Port

	// 3. Detect possible ATA or ATAPI Devices
	int count = 0;
	for (int i = 0; i < 2; i++)
		for(int j = 0; j < 2; j++) {
			uint8_t err = 0, type = IDE_ATA, status;
			ide_devices[count].reserved = 0; // Assume there is no drive here

			// i. Select the Drive
			ideWrite(i, ATA_REG_HDDEVSEL, 0xA0 | (j << 4));
			sleep(1);	// Wait 1ms for drive select to finish

			// ii. Send ATA Identify Command
			ideWrite(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
			sleep(1);	// Wait for 1ms	

			// iii. Polling
			if (ideRead(i, ATA_REG_STATUS) == 0) continue; // If status == 0, No Device
			
			while(1) {
				status = ideRead(i, ATA_REG_STATUS);
				if ((status & ATA_SR_ERR)) { err = 1; break; } // If ERR, device is not ATA
				if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ)) break; // Everything checks
			}

			// iv. Probe for ATAPI Devices
			if (err != 0) {
				uint8_t cl = ideRead(i, ATA_REG_LBA1);
				uint8_t ch = ideRead(i, ATA_REG_LBA2);

				if (cl == 0x14 && ch == 0xEB)
					type = IDE_ATAPI;
				else if (cl == 0x69 && ch == 0x96)
					type = IDE_ATAPI;
				else
					continue; // Type unknown (Might not be a Device)

				ideWrite(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY_PACKET);
				sleep(1); // You know the drill
			}

			// v. Read the Identification Space of the Device	
			ideReadBuffer(i, ATA_REG_DATA, (uint32_t*) ide_buf, 128);	

			// vi. Read Device Parameters
			ide_devices[count].reserved = 1;
			ide_devices[count].type 	= type;
			ide_devices[count].channel = i;
			ide_devices[count].drive = j;
			ide_devices[count].signature = *((uint16_t *)ide_buf + ATA_IDENT_DEVICETYPE);
			ide_devices[count].capabilities = *((uint16_t *)ide_buf + ATA_IDENT_CAPABILITIES);
			ide_devices[count].command_sets = *((uint16_t *)ide_buf + ATA_IDENT_COMMANDSETS);

			// vii. Get Size
			if (ide_devices[count].command_sets & (1 << 26))
				// Device uses 48-bit addressing
				ide_devices[count].size = *((uint16_t*)(ide_buf + ATA_IDENT_MAX_LBA_EXT));
			else
				// Device uses 48-bit addressing
				ide_devices[count].size = *((uint16_t*)(ide_buf + ATA_IDENT_MAX_LBA));

			// viii. String indicated model of device
			for (int k = 0; k < 40; k += 2){
				ide_devices[count].model[k] = ide_buf[ATA_IDENT_MODEL + k + 1];
				ide_devices[count].model[k+1] = ide_buf[ATA_IDENT_MODEL + k];
			}
			ide_devices[count].model[40] = '\0'; // Terminate String
			
			count++;
		}
	// 4. Print Summmary
	for (int i = 0; i < 4; i++)
		if(ide_devices[i].reserved == 1) {
			kprintf("  [%d] Found %s Drive %dGB - %s\n", i,
					(const char*[]){"ATA", "ATAPI"}[ide_devices[i].type],
					ide_devices[i].size / 1024*1024 / 2,
					ide_devices[i].model);
		}
}

extern void ataReadFromPort(uint16_t port, char* buffer, uint32_t num_reads);
extern void ataWriteToPort(uint16_t port, char* buffer, uint32_t num_reads);
// Used by ideRead/write_sectors
uint8_t ideATAAccess(uint8_t dir, uint8_t drive, uint32_t lba,
						uint8_t num_sects, char* buffer) {
    disable_interrupts();
    
	uint8_t lba_mode, 	/* 0: CHS, 1:LBA48 */
			dma, 	   	/* 0: No DMA, 1: DMA */ 
			cmd;
	uint8_t lba_io[6];
	uint32_t channel = ide_devices[drive].channel;
	uint32_t slave_bit = ide_devices[drive].drive;
	uint32_t bus = channels[channel].base; // Bus Base, e.g. 0x1F0, also data port.
	uint32_t words = 256;	// Nearly every ATA drive has a sector-size of 512-bytes
	uint16_t cyl;	// Cylinder
	uint8_t head, sect, err; // Head, Sector, Error

	// Currently, we are not using IRQs, so we must disable them
	// If bit 1 in the control register (nIEN bit) is set, no IRQs will be invoked by any drives
	// on this channel, either master or slave.
	ideWrite(channel, ATA_REG_CONTROL, 
				channels[channel].no_interrupt = (ide_irq_invoked = 0x0) + 0x02);

	// 1. Read the Parameters, select from LBA28, LBA48, or CHS
	if (lba >= 0x10000000) { // LBA 48 is the only one accessible here
        //kprintf("LBA48 mode\n");
		lba_mode = 2;
		lba_io[0] = (lba & 0x000000FF) >> 0;
		lba_io[1] = (lba & 0x0000FF00) >> 8;
		lba_io[2] = (lba & 0x00FF0000) >> 16;
		lba_io[3] = (lba & 0xFF000000) >> 24;
		lba_io[4] = 0; // We are only a 32-bit system 
		lba_io[5] = 0; // We are only a 32-bit system 
		head 	  = 0; // Lower 4-bits of HDDEVSEL are not used here
	} else if (ide_devices[drive].capabilities & 0x200) { // Drive supports LBA?
		// LBA28:
        //kprintf("LBA28 mode\n");
		lba_mode = 1;
		lba_io[0] = (lba & 0x000000FF) >> 0;
		lba_io[1] = (lba & 0x0000FF00) >> 8;
		lba_io[2] = (lba & 0x00FF0000) >> 16;
		lba_io[3] = 0; // These registers are not used here
		lba_io[4] = 0; // These registers are not used here
		lba_io[5] = 0; // These registers are not used here
		head 	  = (lba & 0xF0000000) >> 24; 
	} else { // CHS
		lba_mode  = 0;
		sect 	  = (lba % 63) + 1;
		cyl		  = (lba + 1 - sect) / (16 * 63);
		lba_io[0] = sect;
		lba_io[1] = (cyl >> 0) & 0xFF;
		lba_io[2] = (cyl >> 8) & 0xFF;
		lba_io[3] = 0; 
		lba_io[4] = 0; 
		lba_io[5] = 0; 
		// Head number is writted to HDDEVSEL lower 4-bits
		head 	  = (lba + 1 - sect) % (16 * 63) / (63); 
	}

	// 2. See if drive supports DMA
	dma = 0;

	// 3. Wait if the drive is busy
	while (ideRead(channel, ATA_REG_STATUS) & ATA_SR_BSY);

	/* HDDEVSEL register now looks like this:
	 * bits 0:3 Head number of CHS
	 * bit 4: slave bit (0: master, 1: slave)
	 * bit 5: Obsolete, should be 1
	 * bit 6: LBA (0: CHS, 1: LBA)
	 * bit 7: Obsolete, should be 1
	 */

	// 4. Select Drive from the controller
	if (lba_mode == 0) // Drive & CHS
		ideWrite(channel, ATA_REG_HDDEVSEL, 0xA0 | (slave_bit << 4) | head);
	else // Drive & LBA
		ideWrite(channel, ATA_REG_HDDEVSEL, 0xE0 | (slave_bit << 4) | head);

	// 5. Write Parameters
	if (lba_mode == 2) {
		ideWrite(channel, ATA_REG_SECCOUNT1, 0);
		ideWrite(channel, ATA_REG_LBA3, lba_io[3]);
		ideWrite(channel, ATA_REG_LBA4, lba_io[4]);
		ideWrite(channel, ATA_REG_LBA5, lba_io[5]);
	}
	ideWrite(channel, ATA_REG_SECCOUNT0, num_sects);
	ideWrite(channel, ATA_REG_LBA0, lba_io[0]);
	ideWrite(channel, ATA_REG_LBA1, lba_io[1]);
	ideWrite(channel, ATA_REG_LBA2, lba_io[2]);


	// 6. Select the command and send it
   if (lba_mode == 0 && dma == 0 && dir == 0) cmd = ATA_CMD_READ_PIO;
   else if (lba_mode == 1 && dma == 0 && dir == 0) cmd = ATA_CMD_READ_PIO;   
   else if (lba_mode == 2 && dma == 0 && dir == 0) cmd = ATA_CMD_READ_PIO_EXT;   
   else if (lba_mode == 0 && dma == 1 && dir == 0) cmd = ATA_CMD_READ_DMA;
   else if (lba_mode == 1 && dma == 1 && dir == 0) cmd = ATA_CMD_READ_DMA;
   else if (lba_mode == 2 && dma == 1 && dir == 0) cmd = ATA_CMD_READ_DMA_EXT;
   else if (lba_mode == 0 && dma == 0 && dir == 1) cmd = ATA_CMD_WRITE_PIO;
   else if (lba_mode == 1 && dma == 0 && dir == 1) cmd = ATA_CMD_WRITE_PIO;
   else if (lba_mode == 2 && dma == 0 && dir == 1) cmd = ATA_CMD_WRITE_PIO_EXT;
   else if (lba_mode == 0 && dma == 1 && dir == 1) cmd = ATA_CMD_WRITE_DMA;
   else if (lba_mode == 1 && dma == 1 && dir == 1) cmd = ATA_CMD_WRITE_DMA;
   else cmd = ATA_CMD_WRITE_DMA_EXT;
    
   ideWrite(channel, ATA_REG_COMMAND, cmd);

	// After sending the command, we should poll, then read/write a sector and repeat
	// until all sectors needed, or if an error has occured, the function will
	// return a specific error code
    if(dma){
        

    }else {
        if (dir == 0) {
            for (int i = 0; i < num_sects; i++) {
                if((err = ide_polling(channel, 1))) {
                    return err; // Polling, set error and exit if there is;
                }
                //kprintf("ATA Reading!, buffer: 0x%x, bus: 0x%x\n", buffer, bus);
                for(size_t j = 0; j < words*2; j+=2) {
                    uint16_t tmp = inw(bus);

                    buffer[j] = (tmp & 0xff00) >> 8;
                    buffer[j+1] = tmp & 0xff;
                }
                //ataReadFromPort(bus, buffer, words);
                buffer += (words*2);
            }
        } else {
            for (int i = 0; i < num_sects; i++) {
                ide_polling(channel, 0); // Polling
                //kprintf("ATA Writing!, buffer: 0x%x, bus: 0x%x\n", buffer, bus);
                for(size_t j = 0; j < words*2; j+=2) {
                    uint16_t tmp = ((uint16_t)buffer[j] <<8);
                    tmp |= (uint16_t)buffer[j+1] & 0xff;

                    outw(bus, tmp);
                }
                //ataWriteToPort(bus, buffer, words);
                buffer += (words*2);
            }

            // Flush the cache
            ideWrite(channel, ATA_REG_COMMAND, (char[]) {
                    ATA_CMD_CACHE_FLUSH,
                    ATA_CMD_CACHE_FLUSH,
                    ATA_CMD_CACHE_FLUSH_EXT
                    }[lba_mode]);
            ide_polling(channel, 0); // Polling
        }
    }

    enable_interrupts();

	return 0; // wow thats a mammoth
}

uint8_t ideReadSectors(uint8_t drive, uint8_t num_sects, uint32_t lba,
					char* buffer) {

	// 1. Check if the drive presents:
	if (drive > 3 || ide_devices[drive].reserved == 0) return 0x1; // Drive not found :(
	
	// 2. Check if inputs are valid
	else if( ((lba + num_sects) > ide_devices[drive].size) 
			&& (ide_devices[drive].type = IDE_ATA))
		return 2;	// Seeking to invalid position
	// 3. Read in PIO Mode through Polling
	else {
		uint8_t err = 0;
		if(ide_devices[drive].type == IDE_ATA)
			err = ideATAAccess(ATA_READ, drive, lba, num_sects, buffer);

        ide_print_error(drive, err);	
		return err; 
	}
	return 0;
}

uint8_t ideWriteSectors(uint8_t drive, uint8_t num_sects, uint32_t lba,
						char * buffer) {
	// 1. Check if the drive presents:
	if (drive > 3 || ide_devices[drive].reserved == 0) return 0x1; // Drive not found :(
	
	// 2. Check if inputs are valid
	else if( ((lba + num_sects) > ide_devices[drive].size) 
			&& (ide_devices[drive].type = IDE_ATA))
		return 2;	// Seeking to invalid position
	// 3. Read in PIO Mode through Polling
	else {
		uint8_t err = 0;
		if(ide_devices[drive].type == IDE_ATA)
			err = ideATAAccess(ATA_WRITE, drive, lba, num_sects, buffer);
		else if(ide_devices[drive].type == IDE_ATAPI)
			err = 4; // Write is protected
		return ide_print_error(drive, err);	
	}
}

uint8_t ideReadDMASectors(uint8_t drive, uint16_t num_sects, uint32_t lba, uint32_t* address) {
    prdt[0].address = (uintptr_t) address;
    prdt[0].byte_count = num_sects * 512;
    prdt[0].reserved = 0x8000;


    // Reading code... (send lba bits, set start/stop bit, etc)
}

void ataSetPrdt() {
   // Set bus master prdt register 
}

void ata_test (){
	uint8_t bytes[512];
	uint8_t read[512];
	for(int i = 0; i < 512; i++) read[i] = 0x00;
	for(int i = 0; i < 512; i++) bytes[i] = 0x00;
    char* str = "Hello there paulio";
    kstrcpy((char*)bytes, str);

	uint8_t err = ideWriteSectors(0, 1, 512*8, (char*)bytes);
	if(err == 0)
		kprintf("no write errors\n");
	err = ideReadSectors(0, 1, 512*8, (char*)read);
	if(err == 0) {
		kprintf("no read errors\n");
        kprintf("%s\n", (char*)read);
        kprintf("\n");
    }

}

