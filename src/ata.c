/*
 * The purpose of this file is to interface with ATA and ATAPI devices
 * ATA is common for hard drives and
 * ATAPI is common for optical drives
 */

#include <stdint.h>
#include <stddef.h>
#include <kstdio.h>
#include <io.h>
#include <ata.h>
#include <timer.h>

unsigned char ide_buf[2048] = {0};
static uint8_t ide_irq_invoked = 0;
static uint8_t atapi_packet[12] = {0xA8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
struct ide_device ide_devices[4];
struct IDEChannelRegisters channels[2];


void ide_write(uint8_t channel, uint8_t reg, uint8_t data) {
   if (reg > 0x07 && reg < 0x0C)
      ide_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].no_interrupt);
   if (reg < 0x08)
      outb(channels[channel].base  + reg - 0x00, data);
   else if (reg < 0x0C)
      outb(channels[channel].base  + reg - 0x06, data);
   else if (reg < 0x0E)
      outb(channels[channel].ctrl  + reg - 0x0A, data);
   else if (reg < 0x16)
      outb(channels[channel].bmide + reg - 0x0E, data);
   if (reg > 0x07 && reg < 0x0C)
      ide_write(channel, ATA_REG_CONTROL, channels[channel].no_interrupt);
}

uint8_t ide_read(uint8_t channel, uint8_t reg) {
	uint8_t result = 0;
	if(reg > 0x07 && reg < 0x0c)
		ide_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].no_interrupt);
	else if(reg < 0x08)
		result = inb(channels[channel].base + reg - 0x00);
	else if(reg < 0x0C)
		result = inb(channels[channel].base + reg - 0x06);
	else if(reg < 0x0E)
		result = inb(channels[channel].base + reg - 0x0A);
	else if(reg < 0x16)
		result = inb(channels[channel].base + reg - 0x0E);
	if(reg > 0x07 && reg < 0x0C)
		ide_write(channel, ATA_REG_CONTROL, channels[channel].no_interrupt);
	return result;
}

uint8_t ide_polling (uint8_t channel, uint32_t advanced_check) {
	// 1. Delay 400 nanosecond for BSY to be set
	for(int i = 0; i < 4; i++)
		ide_read(channel, ATA_REG_ALTSTATUS); // This wastes 100ns

	// 2. Wait for BSY to be cleared
	while(ide_read(channel, ATA_REG_STATUS) & ATA_SR_BSY);

	if (advanced_check) {
		uint8_t state = ide_read(channel, ATA_REG_STATUS); 

		// 3. Check for errors
		if (state & ATA_SR_ERR)
			return 2; // Error

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
      unsigned char st = ide_read(ide_devices[drive].channel, ATA_REG_ERROR);
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

extern void read_buffer_begin(void);
extern void insl (uint16_t port, uint32_t buffer, uint32_t num_times);
extern void read_buffer_end(void);
void ide_read_buffer (uint8_t channel, uint8_t reg, uint32_t* buffer,
		uint32_t num_quads) {
	if (reg > 0x07 && reg < 0x0C)
		ide_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].no_interrupt);

	if (reg < 0x08)
		read_insl(channels[channel].base + reg - 0x00, buffer, num_quads);
	else if (reg < 0x0C)
		read_insl(channels[channel].base + reg - 0x06, buffer, num_quads);
	else if (reg < 0x0E)
		read_insl(channels[channel].ctrl + reg - 0x0A, buffer, num_quads);
	else if (reg < 0x16)
		read_insl(channels[channel].bmide + reg - 0x0E, buffer, num_quads);

	if (reg > 0x07 && reg < 0x0C)
		ide_write(channel, ATA_REG_CONTROL, channels[channel].no_interrupt);
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
	ide_write(ATA_PRIMARY  , ATA_REG_CONTROL, 2); // setting nIEN (no_interrupt) in Control Port
	ide_write(ATA_SECONDARY, ATA_REG_CONTROL, 2); // setting nIEN (no_interrupt) in Control Port

	// 3. Detect possible ATA or ATAPI Devices
	int count = 0;
	for (int i = 0; i < 2; i++)
		for(int j = 0; j < 2; j++) {
			uint8_t err = 0, type = IDE_ATA, status;
			ide_devices[count].reserved = 0; // Assume there is no drive here

			// i. Select the Drive
			ide_write(i, ATA_REG_HDDEVSEL, 0xA0 | (j << 4));
			sleep(1);	// Wait 1ms for drive select to finish

			// ii. Send ATA Identify Command
			ide_write(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
			sleep(1);	// Wait for 1ms	

			// iii. Polling
			if (ide_read(i, ATA_REG_STATUS) == 0) continue; // If status == 0, No Device
			
			while(1) {
				status = ide_read(i, ATA_REG_STATUS);
				if ((status & ATA_SR_ERR)) { err = 1; break; } // If ERR, device is not ATA
				if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ)) break; // Everything checks
			}

			// iv. Probe for ATAPI Devices
			if (err != 0) {
				uint8_t cl = ide_read(i, ATA_REG_LBA1);
				uint8_t ch = ide_read(i, ATA_REG_LBA2);

				if (cl == 0x14 && ch == 0xEB)
					type = IDE_ATAPI;
				else if (cl == 0x69 && ch == 0x96)
					type = IDE_ATAPI;
				else
					continue; // Type unknown (Might not be a Device)

				ide_write(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY_PACKET);
				sleep(1); // You know the drill
			}

			// v. Read the Identification Space of the Device	
			ide_read_buffer(i, ATA_REG_DATA, (uint32_t*) ide_buf, 128);	

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




void ata_init() {
}
