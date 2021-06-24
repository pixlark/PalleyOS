/*
 * The purpose of this file is to interface with ATA and ATAPI devices
 * ATA is common for hard drives and
 * ATAPI is common for optical drives
 */

#include <stdint.h>
#include <stddef.h>
#include <kstdio.h>
#include <kstdlib.h>
#include <kheap.h>
#include <io.h>
#include <ata.h>
#include <timer.h>

unsigned char ide_buf[2048] = {0};
static uint8_t ide_irq_invoked = 0;
static uint8_t atapi_packet[12] = {0xA8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
struct IDEDevice ide_devices[4];
struct IDEChannelRegisters channels[2];

static uint8_t dmaArray[1024*64] __attribute__((aligned(64*1024))) = "Nothing Written"; // 64 kib
static PRD prdt[1] __attribute__((aligned (64*1024)));
static PRD test_prd __attribute__ ((aligned (4)));

void ideSendPRDT(uint8_t drive, uint32_t prdt_addr);

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

// Defined in ata_helpers.s
extern void ataReadFromPort(uint16_t port, char* buffer, uint32_t num_reads);
extern void ataWriteToPort(uint16_t port, char* buffer, uint32_t num_reads);
// Used by ideRead/WriteSectors
// if dir=0, read
// if dir=1, write
uint8_t ideATAAccess(uint8_t dir, uint8_t drive, uint32_t lba,
						uint8_t num_sects, char* buffer) {
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
    if(dma) { // DMA setup
    }

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
        uint8_t channel = ide_devices[drive].channel;
        uint16_t base_port = channels[channel].bmide;
        uint8_t oldComRegister = inb(base_port + 0x0);
        outb(base_port + 0x0, oldComRegister | 1); // Set Start/Stop bit (put in DMA mode)

        kprintf("COM Register: 0x%x\n", inb(base_port + 0x0));

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

void printPRDTFromDrive(uint8_t drive) {
    uint8_t channel = ide_devices[drive].channel;
    uint16_t base_port = channels[channel].bmide;
    PRD* prdt = (PRD*)(uintptr_t)inl(base_port+4);
    kprintf("prdt: 0x%x\n", prdt);
    kprintf("prdt[0].address: 0x%x\n", prdt[0].address);
    kprintf("prdt[0].byte_count: 0x%x\n", prdt[0].byte_count);
    kprintf("prdt[0].reserved: 0x%x\n", prdt[0].reserved);
}

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
    if((poll_err = ide_polling(ide_devices[drive].channel, 0)))
        kprintf("Polling Error: %d\n", poll_err); 

    // TODO: Check if device supports DMA, for now we assume that is does

    uint16_t num_sects = size/512;
    uint8_t channel = ide_devices[drive].channel;
    uint16_t base_port = channels[channel].bmide;
    // Clear Bit 0, (stop any DMA transfers)
    uint8_t oldDMA = inb(base_port + 0x0);
    outb(base_port + 0x0, oldDMA & ~(1));


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
    outb(base_port + 0x0, (inb(base_port + 0x0) & ~(1<<3))); // Read, clear RW bit 
    if(inb(base_port) != (old_cr & ~(1<<3)))
        kprintf("ERROR setting Bus Master Command Register\n");

    //4. Clear the Error and Interrupt bit in the BM Status Register
    //   The bits are cleared by writing '1' to it...why????
    uint8_t old_sr = inb(base_port + 2);
    outb(base_port + 0x2, inb(base_port+2) | (4 | 2)); // Clear the error bit in the status register 
    if( inb(base_port+2) != (old_sr & ~(4|2)) )
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
    if((poll_err = ide_polling(ide_devices[drive].channel, 0)))
        kprintf("Polling Error: %d\n", poll_err); 

    //6. Send the LBA and sector count to respective ports
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
    ide_polling(ide_devices[drive].channel, 1);

    //7. Send the DMA transfer command to ATA controller
   ideWrite(channel, ATA_REG_COMMAND, ATA_CMD_READ_DMA);
    if((poll_err = ide_polling(ide_devices[drive].channel, 0)))
        kprintf("Polling Error: %d\n", poll_err); 

    //8. Set Start/Stop bit (bit 0) in BMCR
    uint8_t old = inb(base_port + 0);
    outb(base_port, old | 1);

    uint8_t sr = inb(base_port+2);
    uint8_t active = sr & 1;
    uint8_t interrupt = (sr & 1<<2)>>2;



    while(active && !interrupt){
        kprintf("Waiting..., sr: 0x%x\n", sr);
        sr = inb(base_port+2);
        active = sr & 1;
        interrupt = (sr & 1<<2)>>2;
    }
    kprintf("SR after transfer, active: 0x%x, interrupt: 0x%x\n", active, interrupt);

    if((poll_err = ide_polling(ide_devices[drive].channel, 0)))
        kprintf("Polling Error: %d\n", poll_err); 
    
    outb(base_port+2, 4);
    uint8_t err = inb(base_port+2)&2;
    if(err != 0)
        kprintf("ERROR with DMA read\n");
    err = inb(channels[ide_devices[drive].channel].base + 7) & 1;
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

    uint8_t oldDMA = inb(base_port + 0x0);
    outb(base_port + 0x0, oldDMA & ~(1));
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

    kmemset(read, 0, sizeof(read));
    uintptr_t addr = ataReadDMA(0, 512*4, 512*4); 
    if(addr == NULL) return;
    kprintf("Reading from address\n");
    kprintf("Read: %s\n", (char*)addr);
}

