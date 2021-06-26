/* The purpose of this file is to interface with ATA and ATAPI devices

ATA is common for hard drives and
ATAPI is common for optical drives
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


#define DEVICE_MODEL_STR_LEN 40

#define BMIDE_REG_COMMAND   0x0
#define BMIDE_REG_STATUS    0x2

#define BMIDE_RW        1<<3
#define BMIDE_ERROR_BIT 1<<1
#define BMIDE_INTERRUPT 1<<2
#define BMIDE_ACTIVE    1<<0

#define NUM_IDE_DEVICES     4   // This should be constant
#define NUM_IDE_CHANNELS    2   // This should be constant

#include <stdint.h>
#include <stddef.h>
#include <kstdio.h>
#include <kstdlib.h>
#include <kheap.h>
#include <io.h>
#include <ide.h>
#include <timer.h>

unsigned char ide_buf[2048] = {0};
static uint8_t ide_irq_invoked = 0;
static uint8_t atapi_packet[12] = {0xA8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
IDEDevice ide_devices[NUM_IDE_DEVICES];
IDEChannelRegisters channels[NUM_IDE_CHANNELS];

static uint8_t dmaArray[1024*64] __attribute__((aligned(64*1024))) = "Nothing Written"; // 64 kib
static PRD prdt[1] __attribute__((aligned (64*1024)));
static PRD test_prd __attribute__ ((aligned (4)));

static bool received_interrupt = false;

/* ===== Function Declarations ===== */
static void ideSendPRDT(uint8_t drive, uint32_t prdt_addr);

// Write a byte of data to a Disk Device register
static void ideWrite(uint8_t channel, uint8_t reg, uint8_t data) {
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

// Read a byte of data from a Disk Device register
static uint8_t ideRead(uint8_t channel, uint8_t reg) {
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

// Waits for BSY flag to be cleared
// if advanced_check is true, this will return general errors
static uint8_t idePolling (uint8_t channel, bool advanced_check) {
	// 1. Delay 400 nanosecond for BSY to be set
	for(int i = 0; i < 4; i++)
		ideRead(channel, ATA_REG_ALTSTATUS); // This wastes 100ns

	// 2. Wait for BSY to be cleared
	while(ideRead(channel, ATA_REG_STATUS) & ATA_SR_BSY);

	if (advanced_check) {
		uint8_t state = ideRead(channel, ATA_REG_STATUS); 

		// 3. Check for errors
		if (state & ATA_SR_ERR) {
			return GeneralError; // Error
		}

		// 4. Check if Device Fault
		if (state & ATA_SR_DF)
			return DeviceFault; // Device Fault

		// 5. Check if DRQ (Data Request Ready)
		if ((state & ATA_SR_DRQ) == 0)
			return DRQError; // DRQ should be set
	}

	return 0;
}

ATAError idePrintError (uint8_t drive, ATAError err) {
 if (err == NoError)
      return err;
 
   kprintf("IDE:");
   if (err == DriveNotFound) { kprintf("- Device DriveNotFound\n  "); }
   else if (err == DeviceFault) { kprintf("- Device Fault\n    "); err = 19; }
   else if (err == GeneralError) {
      unsigned char st = ideRead(ide_devices[drive].channel, ATA_REG_ERROR);
      if (st & ATA_ER_AMNF)  { kprintf("- No Address Mark Found\n    ");   		err = 7;  }
      if (st & ATA_ER_TK0NF) { kprintf("- No Media or Media Error\n    ");   	err = 3;  }
      if (st & ATA_ER_ABRT)  { kprintf("- Command Aborted\n    ");      		err = 20; }
      if (st & ATA_ER_MCR)   { kprintf("- No Media or Media Error\n    ");   	err = 3;  }
      if (st & ATA_ER_IDNF)  { kprintf("- ID mark not Found\n    ");      		err = 21; }
      if (st & ATA_ER_MC)    { kprintf("- No Media or Media Error\n     ");   	err = 3;  }
      if (st & ATA_ER_UNC)   { kprintf("- Uncorrectable Data Error\n    ");   	err = 22; }
      if (st & ATA_ER_BBK)   { kprintf("- Bad Sectors\n    ");       			err = 13; }
   } else if (err == DRQError)       { kprintf("- Reads Nothing\n    "); 	    err = 23; }
     else if (err == WriteProtected) { kprintf("- Write Protected\n    ");       err = 8; }

   kprintf("- [%s %s] %s\n",
      (const char *[]){"Primary", "Secondary"}[ide_devices[drive].channel], // Use the channel as an index into the array
      (const char *[]){"Master", "Slave"}[ide_devices[drive].drive], 		// Same as above, using the drive
      ide_devices[drive].model);
 
   return err;
}

void readPortIntoBuffer(uint16_t port, uint32_t* buffer, uint32_t num_times){
	while(num_times--)	{
		*buffer = inl(port);
		buffer++;
	}
}

// Read num_longs into buffer from Drive Device register
void ideReadBuffer (uint8_t channel, uint8_t reg, uint32_t* buffer,
		uint32_t num_longs) {
	if (reg > 0x07 && reg < 0x0C)
		ideWrite(channel, ATA_REG_CONTROL, 0x80 | channels[channel].no_interrupt);

	if (reg < 0x08)
		readPortIntoBuffer(channels[channel].base + reg - 0x00, buffer, num_longs);
	else if (reg < 0x0C)
		readPortIntoBuffer(channels[channel].base + reg - 0x06, buffer, num_longs);
	else if (reg < 0x0E)
		readPortIntoBuffer(channels[channel].ctrl + reg - 0x0A, buffer, num_longs);
	else if (reg < 0x16)
		readPortIntoBuffer(channels[channel].bmide + reg - 0x0E, buffer, num_longs);

	if (reg > 0x07 && reg < 0x0C)
		ideWrite(channel, ATA_REG_CONTROL, channels[channel].no_interrupt);
}


// Gets Drive Device Information through the IDENTIFY command
// and adds to the ide_devices array
void ideInitialize (uint32_t BAR0, uint32_t BAR1, 
                     uint32_t BAR2, uint32_t BAR3, uint32_t BAR4){
	// 1. Detect I/O Ports which interface with IDE Controller
	channels[ATA_PRIMARY  ].base  = (BAR0 & 0xFFFFFFFC) + 0x1F0 * (!BAR0);
	channels[ATA_PRIMARY  ].ctrl  = (BAR1 & 0xFFFFFFFC) + 0x3F6 * (!BAR1);
	channels[ATA_SECONDARY].base  = (BAR2 & 0xFFFFFFFC) + 0x170 * (!BAR2);
	channels[ATA_SECONDARY].ctrl  = (BAR3 & 0xFFFFFFFC) + 0x376 * (!BAR3);
	channels[ATA_PRIMARY  ].bmide = (BAR4 & 0xFFFFFFFC) + 0; // Bus Master IDE
	channels[ATA_SECONDARY].bmide = (BAR4 & 0xFFFFFFFC) + 8; // Bus Master IDE

	// 2. Disable IRQs in both channels
	ideWrite(ATA_PRIMARY  , ATA_REG_CONTROL, 2); // setting nIEN (no_interrupt) in Control Port
	ideWrite(ATA_SECONDARY, ATA_REG_CONTROL, 2); // setting nIEN (no_interrupt) in Control Port

	// 3. Detect possible ATA or ATAPI Devices
	int count = 0;
	for (int channel = 0; channel < 2; channel++) {
		for(int drive = 0; drive < 2; drive++) {
			uint8_t err = 0, type = IDE_ATA, status;
			ide_devices[count].reserved = 0; // Assume there is no drive here

			// i. Select the Drive
			ideWrite(channel, ATA_REG_HDDEVSEL, 0xA0 | (drive << 4));
			sleep(1);	// Wait 1ms for drive select to finish

			// ii. Send ATA Identify Command
			ideWrite(channel, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
			sleep(1);	// Wait for 1ms	

			// iii. Polling
            // If status == 0, No Device
			if (ideRead(channel, ATA_REG_STATUS) == 0) continue; 
			
			while(1) {
				status = ideRead(channel, ATA_REG_STATUS);
		        if ((status & ATA_SR_ERR)) { err = DeviceNotATA; break; } // If ERR, device is not ATA
				if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ)) break; // Everything checks
			}

			// iv. Probe for ATAPI Devices
			if (err != NoError) {
				uint8_t cl = ideRead(channel, ATA_REG_LBA1);
				uint8_t ch = ideRead(channel, ATA_REG_LBA2);

				if      (cl == 0x14 && ch == 0xEB)  type = IDE_ATAPI;
				else if (cl == 0x69 && ch == 0x96)  type = IDE_ATAPI;
				else
					continue; // Type unknown (Might not be a Device)

				ideWrite(channel, ATA_REG_COMMAND, ATA_CMD_IDENTIFY_PACKET);
				sleep(1); // You know the drill
			}

			// v. Read the Identification Space of the Device	
			ideReadBuffer(channel, ATA_REG_DATA, (uint32_t*) ide_buf, 128);	

			// vi. Read Device Parameters
			ide_devices[count].reserved     = 1;
			ide_devices[count].type         = type;
			ide_devices[count].channel      = channel;
			ide_devices[count].drive        = drive;
		    ide_devices[count].signature    = *((uint16_t *)ide_buf + ATA_IDENT_DEVICETYPE);
			ide_devices[count].capabilities = *((uint16_t *)ide_buf + ATA_IDENT_CAPABILITIES);
			ide_devices[count].command_sets = *((uint16_t *)ide_buf + ATA_IDENT_COMMANDSETS);
			ide_devices[count].max_lba      = *((uint16_t *)ide_buf + ATA_IDENT_MAX_LBA);
			ide_devices[count].max_lba_ext   = *((uint16_t *)ide_buf + ATA_IDENT_MAX_LBA_EXT);

			// vii. Get Size
			if (ide_devices[count].command_sets & (1 << 26))
				// Device uses 48-bit addressing
				ide_devices[count].size = *((uint16_t*)(ide_buf + ATA_IDENT_MAX_LBA_EXT));
			else
				// Device uses 48-bit addressing
				ide_devices[count].size = *((uint16_t*)(ide_buf + ATA_IDENT_MAX_LBA));

			// viii. String indicated model of device
			for (int k = 0; k < DEVICE_MODEL_STR_LEN; k += 2){
				ide_devices[count].model[k]   = ide_buf[ATA_IDENT_MODEL + k + 1];
				ide_devices[count].model[k+1] = ide_buf[ATA_IDENT_MODEL + k];
			}
			ide_devices[count].model[DEVICE_MODEL_STR_LEN] = '\0'; // Terminate String
			
			count++;
		}
    }
	// 4. Print Summmary
	for (int i = 0; i < 4; i++) {
		if(ide_devices[i].reserved == 1) {
			kprintf("  [%d] Found %s Drive %dGB - %s\n", i,
					(const char*[]){"ATA", "ATAPI"}[ide_devices[i].type],
					ide_devices[i].size / 1024*1024 / 2,
					ide_devices[i].model);
		}
    }
}

// Sends the LBA address to the Drive Device
static void ideSendLBA(uint16_t channel, uint8_t lba_mode, 
                       uint8_t lba_io[6], uint16_t num_sects) {
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
}

static void ideSendAccessCommand(uint16_t channel, uint8_t lba_mode, uint8_t dma, uint8_t dir){
    uint8_t cmd;
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
}

// Defined in ata_helpers.s
extern void ataReadFromPort(uint16_t port, char* buffer, uint32_t num_reads);
extern void ataWriteToPort(uint16_t port, char* buffer, uint32_t num_reads);
// Used by ideRead/WriteSectors
// if dir=0, read
// if dir=1, write
uint8_t ideATAAccess(uint8_t dir, uint8_t drive, uint32_t lba,
						uint8_t num_sects, char* buffer) {
	uint8_t lba_mode, 	/* 0: CHS, 1:LBA48 */
			dma; 	   	/* 0: No DMA, 1: DMA */ 
	uint8_t lba_io[6];
	uint32_t channel = ide_devices[drive].channel;
	uint32_t slave_bit = ide_devices[drive].drive;
	uint32_t bus = channels[channel].base; // Bus Base, e.g. 0x1F0, also data port.
	uint32_t words = 256;	// Nearly every ATA drive has a sector-size of 512-bytes
	uint16_t cyl;	// Cylinder
	uint8_t head, sect;
    ATAError err; // Head, Sector, Error

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
    if(dma) { // DMA setup
    }

	// 3. Wait if the drive is busy
    idePolling(channel, false);

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
    ideSendLBA(channel, lba_mode, lba_io, num_sects);

	// 6. Select the command and send it
    ideSendAccessCommand(channel, lba, dma, dir);

	// We should poll, then read/write a sector and repeat
	// until all sectors needed, or if an error has occured, the function will
	// return an ATAError 
    if(dma){
        uint8_t channel = ide_devices[drive].channel;
        uint16_t base_port = channels[channel].bmide;
        uint8_t oldComRegister = inb(base_port + 0x0);
        outb(base_port + 0x0, oldComRegister | 1); // Set Start/Stop bit (put in DMA mode)


    }else {
        if (dir == ATA_READ) {
            // PIO read sectors into buffer
            for (int i = 0; i < num_sects; i++) {
                // Polling, set error and exit if there is;
                err = idePolling(channel, true);
                if(err != NoError) return err; 
               
                for(size_t j = 0; j < words*2; j+=2) {
                    // Device takes words (2 bytes), not chars
                    uint16_t tmp = inw(bus);

                    buffer[j] = (tmp & 0xff00) >> 8;
                    buffer[j+1] = tmp & 0xff;
                }

                //ataReadFromPort(bus, buffer, words);
                buffer += (words*2);
            }
        } else {
            // PIO write sectors into buffer
            for (int i = 0; i < num_sects; i++) {
                idePolling(channel, false); // Wait for device to be ready
                for(size_t j = 0; j < words*2; j+=2) {
                    // Device takes words (2 bytes), not chars
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
            idePolling(channel, false);
        }
    }

	return NoError; // wow thats a mammoth
}

// Reads num_sects into buffer
ATAError ideReadSectors(uint8_t drive, uint8_t num_sects, uint32_t lba,
					char* buffer) {

	// 1. Check if the drive presents:
	if (drive > 3 || ide_devices[drive].reserved == 0) return DriveNotFound;
	
	// 2. Check if inputs are valid
	else if( lba + num_sects > ide_devices[drive].size &&
			ide_devices[drive].type == IDE_ATA) {
		return LBAOutOfRange;	// Seeking to invalid position
    }

	// 3. Read in PIO Mode through Polling
	else {
		uint8_t err = NoError;
		if(ide_devices[drive].type == IDE_ATA)
			err = ideATAAccess(ATA_READ, drive, lba, num_sects, buffer);

        idePrintError(drive, err);	
		return err; 
	}
	return NoError;
}

// Writes num_sects from buffer to drive
ATAError ideWriteSectors(uint8_t drive, uint8_t num_sects, uint32_t lba,
						char * buffer) {
	// 1. Check if the drive presents:
	if (drive > NUM_IDE_DEVICES-1 || ide_devices[drive].reserved == 0) 
        return DriveNotFound;
	
	// 2. Check if inputs are in range and ATA
	else if( lba + num_sects > ide_devices[drive].size &&
             ide_devices[drive].type == IDE_ATA ) {
		return LBAOutOfRange;	// Seeking to invalid position
    }

	// 3. Read in PIO Mode
	else {
		uint8_t err = NoError;
		if(ide_devices[drive].type == IDE_ATA) {
			err = ideATAAccess(ATA_WRITE, drive, lba, num_sects, buffer);
        } else if(ide_devices[drive].type == IDE_ATAPI) {
			err = WriteProtected; // Write is protected
        }

		return idePrintError(drive, err);	
	}
}

void printPRDTFromDrive(uint8_t drive) {
    uint8_t channel = ide_devices[drive].channel;
    uint16_t base_port = channels[channel].bmide;
    PRD* prdt = (PRD*)(uintptr_t)inl(base_port+4);
    kprintf("prdt: 0x%x\n", prdt);
    kprintf("prdt[0].address: 0x%x\n", prdt[0].address);
    kprintf("prdt[0].byte_count: 0x%x\n", prdt[0].byte_count);
    kprintf("prdt[0].reserved: 0x%x\n", prdt[0].reserved);
}

// Sends Physical Region Descriptor Table address to DMA supporting device
void ideSendPRDT(uint8_t drive, uint32_t prdt_addr) {
   // Set bus master prdt register 
    uint8_t channel = ide_devices[drive].channel;
    uint16_t base_port = channels[channel].bmide;
    outl(base_port + 4, prdt_addr);
    uint32_t set_prdt = inl(base_port+4);
    if(set_prdt != prdt_addr)
        kprintf("ERROR setting PRDT on drive %d, BMIDE: 0x%x\n", drive, base_port);
}

// Reads from ATA drive using DMA.
// Returns the address where to find the data after the transfer
uintptr_t ataReadDMA(uint8_t drive, uint32_t size, uint32_t lba){

    if (size > 1024*64){
        kprintf("Size too big for current implementation\n");
        return (uintptr_t)NULL;
    }

    uint8_t poll_err = 0;
    if((poll_err = idePolling(ide_devices[drive].channel, false)))
        kprintf("Polling Error: %d\n", poll_err); 

    // TODO: Check if device supports DMA, for now we assume that is does

    uint16_t num_sects = size/512;
    uint8_t channel = ide_devices[drive].channel;
    uint16_t base_port = channels[channel].bmide;
    // Clear Bit 0, (stop any DMA transfers)
    uint8_t oldDMA = inb(base_port + BMIDE_REG_COMMAND);
    outb(base_port + BMIDE_REG_COMMAND, oldDMA & ~(1));


//    PRD* localPrdt = (PRD*)kheapAlignedAlloc(1024*64, 1024*64);
     //1. Prepare a PRDT in system memory
    test_prd.address = (uintptr_t)dmaArray;
    test_prd.byte_count = size;
    test_prd.reserved = 0x8000;
    prdt[0].address = (uintptr_t)dmaArray;
    prdt[0].byte_count = size; 
    prdt[0].reserved = 0x8000; // This would be reserved, but since 
                               // it is the last PRD entry, the MSB is set
    
    //2. Send the physical PRDT address to the Bus Master PRDT Register
    kprintf("&prdt[0]: 0x%x\n", &prdt[0]);
    kprintf("prdt: 0x%x\n", prdt);
    kprintf("&test_prd: 0x%x\n", test_prd);
    kprintf("&dmaArray: 0x%x\n", dmaArray);
    ideSendPRDT(drive, &test_prd);
    printPRDTFromDrive(drive);

    //3. Set the direction of the transfer, clear the RW bit in
    //   Bus Master Command Register
    uint8_t old_cr = inb(base_port);
    outb(base_port + BMIDE_REG_COMMAND, 
            inb(base_port + BMIDE_REG_COMMAND) & ~(BMIDE_RW) ); // Read, clear RW bit 

    // Check that it was cleared correctly
    if(inb(base_port) != (old_cr & ~(BMIDE_RW)))
        kprintf("ERROR setting Bus Master Command Register\n");

    //4. Clear the Error and Interrupt bit in the BM Status Register
    //   The bits are cleared by writing '1' to them...why????
    uint8_t old_sr = inb(base_port + BMIDE_REG_STATUS);
    outb(base_port + BMIDE_REG_STATUS, 
            inb(base_port + BMIDE_REG_STATUS) | (BMIDE_ERROR_BIT | BMIDE_ERROR_BIT)); 

    // Check that they were cleared correctly
    if(inb(base_port) != (old_cr & ~(BMIDE_RW)))
    if( inb(base_port+BMIDE_REG_STATUS) != (old_sr & ~(BMIDE_ERROR_BIT | BMIDE_INTERRUPT)) )
        kprintf("ERROR setting Bus Master Status Register\n");

    //5. Select the Drive
    uint8_t lba_mode = 0;
    uint8_t lba_io[6];
    uint16_t cyl;
    uint8_t sect, head;
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
    }else { // CHS
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


	uint32_t slave_bit = ide_devices[drive].drive;
    // Selecting drive here.
    ideWrite(channel, ATA_REG_HDDEVSEL, 0xE0 | (slave_bit << 4) | head);
    if((poll_err = idePolling(ide_devices[drive].channel, false)))
        kprintf("Polling Error: %d\n", poll_err); 

    //6. Send the LBA and sector count to respective ports
    ideSendLBA(channel, lba, lba_io, num_sects);
    
    //7. Send the DMA transfer command to ATA controller
   ideWrite(channel, ATA_REG_COMMAND, ATA_CMD_READ_DMA);
    if((poll_err = idePolling(ide_devices[drive].channel, false)))
        kprintf("Polling Error: %d\n", poll_err); 

    //8. Set Start/Stop bit (bit 0) in BMCR
    uint8_t old = inb(base_port + 0);
    outb(base_port, old | 1);

    uint8_t sr = inb(base_port + BMIDE_REG_STATUS);
    uint8_t active = sr & BMIDE_ACTIVE;
    uint8_t interrupt = (sr & BMIDE_INTERRUPT)>>2;

    while(!received_interrupt);

    kprintf("SR after transfer, active: 0x%x, interrupt: 0x%x\n", active, interrupt);

    if((poll_err = idePolling(ide_devices[drive].channel, false)))
        kprintf("Polling Error: %d\n", poll_err); 
    
    outb(base_port + BMIDE_REG_STATUS, 4);
    uint8_t err = inb(base_port + BMIDE_REG_STATUS) & BMIDE_ERROR_BIT;
    if(err != 0)
        kprintf("ERROR with DMA read\n");
    err = inb(channels[ide_devices[drive].channel].base + ATA_REG_STATUS) & ATA_SR_ERR;

    if(err != 0)
        kprintf("ERROR with drive read, sr: 0x%x\n", err);

   return prdt[0].address;
}

void ideIRQHandler() {
    //1. Reset the Start/Stop bit
    //2. Read controller and drive status to determine if successful
    //3. Set flag to say transfer complete

    kprintf("Done DMA Transfer!!!!!!!!!!!!!\n");
    uint8_t channel = ide_devices[0].channel;
    uint16_t base_port = channels[channel].bmide;

    uint8_t oldDMA = inb(base_port + BMIDE_REG_COMMAND);
    outb(base_port + BMIDE_REG_COMMAND, oldDMA & ~(1));

    received_interrupt = true;
}

void ata_test (){
    uint8_t drive = 0;
    uint8_t channel = ide_devices[drive].channel;
    kprintf("Drive: %d, Channel: %d, BMIDE: 0x%x\n",drive, channel, channels[channel].bmide);
    char write[512*4] = "WOOOOO";
    ideWriteSectors(0, 4, 512*4, write); 
    char read[512*4];
    kmemset(read, 0, sizeof(read));
    ideReadSectors(0, 4, 512*4, read);
    kprintf("should be: %s\n", read);

    //kmemset(read, 0, sizeof(read));
    //uintptr_t addr = ataReadDMA(0, 512*4, 512*4); 
    //if(addr == NULL) return;
    //kprintf("Reading from address\n");
    //kprintf("Read: %s\n", (char*)addr);
}

