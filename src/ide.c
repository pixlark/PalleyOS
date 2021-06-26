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
 
#define DEVICE_MODEL_STR_LEN 40

#define BMIDE_REG_COMMAND   0x0
#define BMIDE_REG_STATUS    0x2

#define BMIDE_RW        1<<3
#define BMIDE_ERROR_BIT 1<<1
#define BMIDE_INTERRUPT 1<<2
#define BMIDE_ACTIVE    1<<0
#define BMIDE_START_STOP 1<<0

#define NUM_IDE_DEVICES     4   // This should be constant
#define NUM_IDE_CHANNELS    2   // This should be constant
#define IDE_SECTOR_SIZE   512

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
static ATAError idePolling (uint8_t channel, bool advanced_check) {
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

static void idePrintDevices() {
	for (int i = 0; i < 4; i++) {
		if(ide_devices[i].reserved == 0) continue;

        kprintf("  [%d] Found %s Drive %dGB - %s\n", i,
                (const char*[]){"ATA", "ATAPI"}[ide_devices[i].type],
                ide_devices[i].size / 1024*1024 / 2,
                ide_devices[i].model);
    }
}

static void ideDeviceInitialize (uint8_t channel, uint8_t drive) {
    static uint8_t count = 0;
    uint8_t err = 0, type = IDE_ATA, status;
    ide_devices[count].reserved = 0; // Assume there is no j here

    // i. Select the Drive
    ideWrite(channel, ATA_REG_HDDEVSEL, 0xA0 | (drive << 4));
    pitSleep(1);	// Wait 1ms for j select to finish

    // ii. Send ATA Identify Command
    ideWrite(channel, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
    pitSleep(1);	// Wait for 1ms	

    // iii. Polling
    // If status == 0, No Device
    if (ideRead(channel, ATA_REG_STATUS) == 0) return; 
    
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
            return; // Type unknown (Might not be a Device)

        ideWrite(channel, ATA_REG_COMMAND, ATA_CMD_IDENTIFY_PACKET);
        pitSleep(1); // You know the drill
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
    ide_devices[count].command_sets = *((uint32_t *)ide_buf + ATA_IDENT_COMMANDSETS);

    // vii. Get Size
    bool lba_extended_support = (ide_devices[count].command_sets & (1 << 26)) != 0;
    if (lba_extended_support) {
        uint32_t lba48_size = *((uint32_t*)(ide_buf + ATA_IDENT_MAX_LBA_EXT));
        ide_devices[count].size = lba48_size;
    } else {
        uint32_t lba28_size = *((uint32_t*)(ide_buf + ATA_IDENT_MAX_LBA));
        ide_devices[count].size = lba28_size;
    }

    // viii. String indicated model of device
    for (int k = 0; k < DEVICE_MODEL_STR_LEN; k += 2){
        ide_devices[count].model[k]   = ide_buf[ATA_IDENT_MODEL + k + 1];
        ide_devices[count].model[k+1] = ide_buf[ATA_IDENT_MODEL + k];
    }
    ide_devices[count].model[DEVICE_MODEL_STR_LEN] = '\0'; // Terminate String
    
    count++;
}

static void setIOPortsFromBars (uint32_t bars[6]) {
	channels[ATA_PRIMARY  ].base  = (bars[0] & 0xFFFFFFFC) + 0x1F0 * (!bars[0]);
	channels[ATA_PRIMARY  ].ctrl  = (bars[1] & 0xFFFFFFFC) + 0x3F6 * (!bars[1]);
	channels[ATA_SECONDARY].base  = (bars[2] & 0xFFFFFFFC) + 0x170 * (!bars[2]);
	channels[ATA_SECONDARY].ctrl  = (bars[3] & 0xFFFFFFFC) + 0x376 * (!bars[3]);
	channels[ATA_PRIMARY  ].bmide = (bars[4] & 0xFFFFFFFC) + 0; // Bus Master IDE
	channels[ATA_SECONDARY].bmide = (bars[5] & 0xFFFFFFFC) + 8; // Bus Master IDE
}

// Gets Drive Device Information through the IDENTIFY command
// and adds to the ide_devices array
void ideInitialize (uint32_t bars[6]) {

	// 1. Detect I/O Ports which interface with IDE Controller
    setIOPortsFromBars(bars);

	// 2. Disable IRQs in both channels
//	ideWrite(ATA_PRIMARY  , ATA_REG_CONTROL, 2); // setting nIEN (no_interrupt) in Control Port
//	ideWrite(ATA_SECONDARY, ATA_REG_CONTROL, 2); // setting nIEN (no_interrupt) in Control Port

	// 3. Detect possible ATA or ATAPI Devices
	for (int channel = 0; channel < 2; channel++) {
		for(int drive = 0; drive < 2; drive++) {
            ideDeviceInitialize(channel, drive);			
		}
    }

	// 4. Print Summmary
    idePrintDevices();
}

static void waitForBSYToClear(uint8_t drive) {
    idePolling(ide_devices[drive].channel, false);
}

static void stopCurrentDMATransfers(uint16_t bmide_base_port) {
    // Clear Bit 0, (stop any DMA transfers)
    uint8_t oldDMA = inb(bmide_base_port + BMIDE_REG_COMMAND);
    outb(bmide_base_port + BMIDE_REG_COMMAND, oldDMA & ~(1));

}
static void startDMATransfer(uint16_t bmide_base_port) {
    uint8_t old_command_register = inb(bmide_base_port + BMIDE_REG_COMMAND);
    outb(bmide_base_port, old_command_register | BMIDE_START_STOP);
}


// Returns the physical memory address of the PRDT to send to Disk Drive
static uintptr_t preparePRDT(uint32_t size) {
//    PRD* localPrdt = (PRD*)kheapAlignedAlloc(1024*64, 1024*64);
    prdt[0].address = (uintptr_t)dmaArray;
    prdt[0].byte_count = size; 
    prdt[0].reserved = 0x8000; // This would be reserved, but since 
                               // it is the last PRD entry, the MSB is set
    return (uintptr_t)(void*)&prdt[0];
}

static void setBmideDMADirection(uint16_t bmide_base_port, IDEDirection dir) {

    uint8_t old_command_register = inb(bmide_base_port);
    uint8_t new_command_register = old_command_register;

    if(dir == IDERead) {
        new_command_register &= ~(BMIDE_RW); // Read, Clear the RW bit
    } else if(dir == IDEWrite) {
        new_command_register |= BMIDE_RW;    // Write, Set the RW bit
    }

    outb(bmide_base_port + BMIDE_REG_COMMAND, new_command_register); 
}

static void clearBmideErrorAndInterrupt(uint16_t bmide_base_port) {
    uint8_t old_status_register = inb(bmide_base_port + BMIDE_REG_STATUS);

    //   The bits are cleared by writing '1' to them...why????
    uint8_t new_status_register = old_status_register | BMIDE_ERROR_BIT | BMIDE_INTERRUPT;
    outb(bmide_base_port + BMIDE_REG_STATUS, new_status_register); 
}

static void generateLBAIO(uint8_t drive, IDETransferInfo* transfer_info) { 
	if (transfer_info->lba >= 0x10000000) { 
 
		transfer_info->lba_mode  = LBA48;
		transfer_info->lba_io[0] = (transfer_info->lba & 0x000000FF) >> 0;
		transfer_info->lba_io[1] = (transfer_info->lba & 0x0000FF00) >> 8;
		transfer_info->lba_io[2] = (transfer_info->lba & 0x00FF0000) >> 16;
		transfer_info->lba_io[3] = (transfer_info->lba & 0xFF000000) >> 24;
		transfer_info->lba_io[4] = 0; // We are only a 32-bit system 
		transfer_info->lba_io[5] = 0; // We are only a 32-bit system 
		transfer_info->head 	 = 0; // Lower 4-bits of HDDEVSEL are not used here

	} else if (ide_devices[drive].capabilities & 0x200) { // Drive supports LBA?

		transfer_info->lba_mode = LBA28;
		transfer_info->lba_io[0] = (transfer_info->lba & 0x000000FF) >> 0;
		transfer_info->lba_io[1] = (transfer_info->lba & 0x0000FF00) >> 8;
		transfer_info->lba_io[2] = (transfer_info->lba & 0x00FF0000) >> 16;
		transfer_info->lba_io[3] = 0; // These registers are not used here
		transfer_info->lba_io[4] = 0; // These registers are not used here
		transfer_info->lba_io[5] = 0; // These registers are not used here
		transfer_info->head 	 = (transfer_info->lba & 0xF0000000) >> 24; 
        
	} else { 

		transfer_info->lba_mode  = LBACHS;
		transfer_info->sect 	 = (transfer_info->lba % 63) + 1;
		transfer_info->cyl		 = (transfer_info->lba + 1 - transfer_info->sect) / (16 * 63);
		transfer_info->lba_io[0] = transfer_info->sect;
		transfer_info->lba_io[1] = (transfer_info->cyl >> 0) & 0xFF;
		transfer_info->lba_io[2] = (transfer_info->cyl >> 8) & 0xFF;
		transfer_info->lba_io[3] = 0; 
		transfer_info->lba_io[4] = 0; 
		transfer_info->lba_io[5] = 0; 
		// Head number is writted to HDDEVSEL lower 4-bits
		transfer_info->head 	  = (transfer_info->lba + 1 - transfer_info->sect) % (16 * 63) / (63); 

	}
}

static void selectDrive(uint8_t drive, IDETransferInfo* transfer_info) {
    uint8_t channel = ide_devices[drive].channel;
	uint32_t slave_bit = ide_devices[drive].drive;
	if (transfer_info->lba_mode == LBACHS) 
		ideWrite(channel, ATA_REG_HDDEVSEL, 0xA0 | (slave_bit << 4) | transfer_info->head);
	else 
		ideWrite(channel, ATA_REG_HDDEVSEL, 0xE0 | (slave_bit << 4) | transfer_info->head);

    waitForBSYToClear(ide_devices[drive].channel);
}

static void sendTranferCommand (uint8_t drive, IDETransferInfo* transfer_info) {
    uint8_t cmd;
    LBAMode lba_mode = transfer_info->lba_mode;
    bool dma = transfer_info->dma;
    IDEDirection dir = transfer_info->dir;

    if      (lba_mode == LBACHS && dma == false && dir == IDERead)  cmd = ATA_CMD_READ_PIO;
    else if (lba_mode == LBA28  && dma == false && dir == IDERead)  cmd = ATA_CMD_READ_PIO;   
    else if (lba_mode == LBA48  && dma == false && dir == IDERead)  cmd = ATA_CMD_READ_PIO_EXT;   
    else if (lba_mode == LBACHS && dma == true  && dir == IDERead)  cmd = ATA_CMD_READ_DMA;
    else if (lba_mode == LBA28  && dma == true  && dir == IDERead)  cmd = ATA_CMD_READ_DMA;
    else if (lba_mode == LBA48  && dma == true  && dir == IDERead)  cmd = ATA_CMD_READ_DMA_EXT;
    else if (lba_mode == LBACHS && dma == false && dir == IDEWrite) cmd = ATA_CMD_WRITE_PIO;
    else if (lba_mode == LBA28  && dma == false && dir == IDEWrite) cmd = ATA_CMD_WRITE_PIO;
    else if (lba_mode == LBA48  && dma == false && dir == IDEWrite) cmd = ATA_CMD_WRITE_PIO_EXT;
    else if (lba_mode == LBACHS && dma == true  && dir == IDEWrite) cmd = ATA_CMD_WRITE_DMA;
    else if (lba_mode == LBA28  && dma == true  && dir == IDEWrite) cmd = ATA_CMD_WRITE_DMA;
    else cmd = ATA_CMD_WRITE_DMA_EXT;

    ideWrite(ide_devices[drive].channel, ATA_REG_COMMAND, cmd);
}


// Sends the LBA address to the Drive Device
static void ideSendLBA(uint8_t drive, uint16_t num_sects, IDETransferInfo* transfer_info) {
    uint8_t channel = ide_devices[drive].channel;
	if (transfer_info->lba_mode == LBA48) {
		ideWrite(channel, ATA_REG_SECCOUNT1, 0);
		ideWrite(channel, ATA_REG_LBA3, transfer_info->lba_io[3]);
		ideWrite(channel, ATA_REG_LBA4, transfer_info->lba_io[4]);
		ideWrite(channel, ATA_REG_LBA5, transfer_info->lba_io[5]);
	}
	ideWrite(channel, ATA_REG_SECCOUNT0, num_sects);
	ideWrite(channel, ATA_REG_LBA0, transfer_info->lba_io[0]);
	ideWrite(channel, ATA_REG_LBA1, transfer_info->lba_io[1]);
	ideWrite(channel, ATA_REG_LBA2, transfer_info->lba_io[2]);
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

static void ideFlushCache(uint8_t drive, IDETransferInfo* transfer_info) {
    uint8_t channel = ide_devices[drive].channel;
    ideWrite(channel, ATA_REG_COMMAND, (char[]) {
                                        ATA_CMD_CACHE_FLUSH,
                                        ATA_CMD_CACHE_FLUSH,
                                        ATA_CMD_CACHE_FLUSH_EXT
                                        }[transfer_info->lba_mode]);
}

// TODO: make sure buffer isn't pushed on the stack
static void ideWriteSectorsFromBuffer(uint8_t drive, uint16_t num_sects, char* buffer) {
    uint8_t channel = ide_devices[drive].channel;
    uint16_t port = channels[channel].base;
    // PIO write sectors into buffer
    for (int i = 0; i < num_sects; i++) {
        idePolling(channel, false); // Wait for device to be ready

        for(size_t j = 0; j < IDE_SECTOR_SIZE; j+=2) {
            // Device takes words (2 bytes), not chars
            uint16_t tmp = ((uint16_t)buffer[j] << 8);
            tmp |= (uint16_t)buffer[j+1] & 0xff;

            outw(port, tmp);
        }

        buffer += IDE_SECTOR_SIZE;
    }
}

static ATAError ideReadSectorsFromBuffer(uint8_t drive, uint16_t num_sects, char* buffer){
    uint8_t channel = ide_devices[drive].channel;
    uint16_t port = channels[channel].base;
    ATAError err = NoError;
    // PIO read sectors into buffer
    for (int i = 0; i < num_sects; i++) {
        err = idePolling(channel, true);
        if(err != NoError) return err; 
       
        for(size_t j = 0; j < IDE_SECTOR_SIZE; j+=2) {
            // Device takes words (2 bytes), not chars
            uint16_t tmp = inw(port);

            buffer[j] = (tmp & 0xff00) >> 8;
            buffer[j+1] = tmp & 0xff;
        }
        buffer += IDE_SECTOR_SIZE;
    }
    return err;
}

// Defined in ata_helpers.s
extern void ataReadFromPort(uint16_t port, char* buffer, uint32_t num_reads);
extern void ataWriteToPort(uint16_t port, char* buffer, uint32_t num_reads);

// Provides generic access to ATA Device
uint8_t ideATAAccess(IDEDirection dir, uint8_t drive, uint32_t lba,
						uint8_t num_sects, char* buffer) {
    IDETransferInfo transfer_info = {0};
    transfer_info.lba = lba;
    transfer_info.dir = dir;
    transfer_info.dma = false;

    generateLBAIO(drive, &transfer_info);

    waitForBSYToClear(drive);

    selectDrive(drive, &transfer_info);
    ideSendLBA(drive, num_sects, &transfer_info);
    sendTranferCommand(drive, &transfer_info);

    if(transfer_info.dma){
        return NoError;
    }

    if (dir == IDERead) {
        return ideReadSectorsFromBuffer(drive, num_sects, buffer);
    } else {
        ideWriteSectorsFromBuffer(drive, num_sects, buffer);
        ideFlushCache(drive, &transfer_info);
        waitForBSYToClear(drive);
    }

	return NoError; 
}

// Reads num_sects into buffer
ATAError ideReadSectors(uint8_t drive, uint8_t num_sects, uint32_t lba,
					char* buffer) {

	// 1. Check if the drive presents:
	if (drive > 3 || ide_devices[drive].reserved == 0) {
        return idePrintError(drive, DriveNotFound);
    }
	
	// 2. Check if inputs are valid
	else if( (lba + num_sects) > ide_devices[drive].size) {
        kprintf("ERROR: LBAOutOfRange, lba 0x%x, size: 0x%x\n", lba, ide_devices[drive].size);
		return LBAOutOfRange;
    }

	// 3. Read in PIO Mode through Polling
	else {
		uint8_t err = NoError;
		if(ide_devices[drive].type == IDE_ATA)
			err = ideATAAccess(IDERead, drive, lba, num_sects, buffer);

        return idePrintError(drive, err);	
	}
	return NoError;
}

// Writes num_sects from buffer to drive
ATAError ideWriteSectors(uint8_t drive, uint8_t num_sects, uint32_t lba,
						char * buffer) {
	// 1. Check if the drive presents:
	if (drive > 3 || ide_devices[drive].reserved == 0) 
        return idePrintError(drive, DriveNotFound);
	
	// 2. Check if inputs are in range and ATA
	else if( (lba + num_sects) > ide_devices[drive].size) {
        kprintf("ERROR: LBAOutOfRange, lba 0x%x, size: 0x%x\n", lba, ide_devices[drive].size);
		return LBAOutOfRange;
    }

	// 3. Read in PIO Mode
	else {
		uint8_t err = NoError;
		if(ide_devices[drive].type == IDE_ATA) {
			err = ideATAAccess(IDEWrite, drive, lba, num_sects, buffer);
        } else if(ide_devices[drive].type == IDE_ATAPI) {
			err = WriteProtected; // Write is protected
        }

		return idePrintError(drive, err);	
	}
}

static void printPRDTFromDrive(uint8_t drive) {
    uint8_t channel = ide_devices[drive].channel;
    uint16_t base_port = channels[channel].bmide;
    PRD* prdt = (PRD*)(uintptr_t)inl(base_port+4);
    kprintf("prdt: 0x%x\n", prdt);
    kprintf("prdt[0].address: 0x%x\n", prdt[0].address);
    kprintf("prdt[0].byte_count: 0x%x\n", prdt[0].byte_count);
    kprintf("prdt[0].reserved: 0x%x\n", prdt[0].reserved);
}

// Sends Physical Region Descriptor Table address to DMA supporting device
static void ideSendPRDT(uint8_t drive, uint32_t prdt_addr) {
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

    waitForBSYToClear(drive);

    // TODO: Check if device supports DMA, for now we assume that is does

    uint16_t num_sects = (size/512);
    uint8_t  channel = ide_devices[drive].channel;
    uint16_t bmide_base_port = channels[channel].bmide;

    stopCurrentDMATransfers(bmide_base_port);

    uintptr_t prdt_loc = preparePRDT(size);
    
    ideSendPRDT(drive, prdt_loc);

    kprintf("prdt_loc: 0x%x\n", prdt_loc);
    printPRDTFromDrive(drive);

    setBmideDMADirection(bmide_base_port, IDERead);
    clearBmideErrorAndInterrupt(bmide_base_port);
    
    IDETransferInfo transfer_info = {0};
    transfer_info.lba = lba;
    transfer_info.dir = IDERead;
    transfer_info.dma = true;

    generateLBAIO(drive, &transfer_info);

    selectDrive(drive, &transfer_info);
    ideSendLBA(drive, num_sects, &transfer_info);

    sendTranferCommand(drive, &transfer_info);
    waitForBSYToClear(drive);

    startDMATransfer(bmide_base_port);

    while(!received_interrupt);

    waitForBSYToClear(drive);    

    outb(bmide_base_port + BMIDE_REG_STATUS, 4);
    uint8_t err = inb(bmide_base_port + BMIDE_REG_STATUS) & BMIDE_ERROR_BIT;
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

    stopCurrentDMATransfers(base_port);

    received_interrupt = true;
}

void ata_test (){
    uint8_t drive = 0;
    uint8_t channel = ide_devices[drive].channel;
    kprintf("Drive: %d, Channel: %d, BMIDE: 0x%x\n",drive, channel, channels[channel].bmide);

    uint32_t lba = 512*4;

    char write[512] = "WOOOOO";
    ideWriteSectors(0, 1, lba, write);

    char read[512];
    kmemset(read, 0, sizeof(read));
    uintptr_t addr = ataReadDMA(0, sizeof(read), lba); 
    if(addr == NULL) return;
    kprintf("Reading from address\n");
    kprintf("Read: %s\n", (char*)addr);
}

