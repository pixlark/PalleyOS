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

#define PBUS 	 0x1F0	// Primary Bus I/O
#define PBUSC 	0x3f6	// Primary Bus Control

#define SBUS	0x170
#define SBUSC	0x376

#define IDENTIFY_SUCCESS		0
#define IDENTIFY_NO_DRIVE	   1
#define IDENTIFY_NOT_ATA		2
#define IDENTIFY_BUS_ERR        3
#define IDENTIFY_DRIVE_ATAPI    4
#define IDENTIFY_DRIVE_SATA 	5
#define IDENTIFY_DRIVE_ABORT    6
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

#define ATA_PRIMARY_SELECT     0xE0
#define ATA_SECONDARY_SELECT     0xF0

/* ===== Interface Types ===== */
#define IDE_ATA        0x00
#define IDE_ATAPI      0x01
 
#define ATA_MASTER     0x00
#define ATA_SLAVE      0x01

/* ===== Register Offsets ===== */
#define ATA_REG_DATA         0x00
#define ATA_REG_ERROR        0x01
#define ATA_REG_FEATURES     0x01
#define ATA_REG_SECCOUNT0    0x02
#define ATA_REG_LBALOW       0x03
#define ATA_REG_LBAMID       0x04
#define ATA_REG_LBAHIGH      0x05
#define ATA_REG_DRIVESELECT  0x06
#define ATA_REG_COMMAND      0x07
#define ATA_REG_STATUS       0x07
#define ATA_REG_SECCOUNT     0x08
#define ATA_REG_LBA3         0x09
#define ATA_REG_LBA4         0x0A
#define ATA_REG_LBA5         0x0B
#define ATA_REG_CONTROL      0x0C
#define ATA_REG_ALTSTATUS    0x0C
#define ATA_REG_DEVADDRESS   0x0D

/* ===== Channels / Directions ===== */
// Channels:
#define ATA_PRIMARY      0x00
#define ATA_SECONDARY    0x01
 
#define DEVICE_MODEL_STR_LEN 40

#define BMIDE_REG_COMMAND   0x0
#define BMIDE_REG_STATUS    0x2

#define BMIDE_RW        1<<3
#define BMIDE_ERROR_BIT 1<<1
#define BMIDE_INTERRUPT 1<<2
#define BMIDE_ACTIVE    1<<0
#define BMIDE_START_STOP 1<<0

#define NUM_IDE_CHANNELS    2   // This should be constant
#define SECTOR_SIZE   512

#include <stdint.h>
#include <stddef.h>
#include <kstdio.h>
#include <kstdlib.h>
#include <kheap.h>
#include <io.h>
#include <ide.h>
#include <timer.h>

typedef struct {
    unsigned int error : 1; // ERR
    unsigned int index_mark : 1; // IDX
    unsigned int data_corrected : 1; // CORR
    unsigned int data_transfer_requested : 1; //DRQ
    unsigned int seek_complete : 1; // DSC
    unsigned int device_fault : 1; // DF
    unsigned int device_ready : 1; // DRDY
    unsigned int busy : 1; // BSY
} ATA_Status_Register;

typedef struct {
    unsigned int bad_block : 1; // BBK
    unsigned int uncorrectable_data_error : 1; // UNC
    unsigned int media_changed : 1; // MC
    unsigned int id_mark_not_found : 1; // IDNF
    unsigned int media_change_requested : 1; // MCR
    unsigned int command_aborted : 1; // TK0NF
    unsigned int track_0_not_found : 1; // AMNF
    unsigned int address_mark_not_found : 1;
} ATA_Error_Register;

typedef enum {
    ATA,
    ATAPI
} ATA_Type;

typedef enum {
    PRIMARY,
    SECONDARY
} ATA_Drive_Type;

typedef struct {
    ATA_Type ata_type;
    ATA_Drive_Type type;
    uint16_t io_port;
    uint16_t command_port;
    bool uses_packets;
    uint32_t num_sectors;
} ATA_Drive;

extern void readStatus();

bool interrupt_recieved = false;


// Holds primary and secondary bus info for ata drives
ATA_Drive ata_drives[2];

static bool isFloatingBus() {
    uint8_t init_read_primary = inb(PBUS + ATA_REG_STATUS);
    uint8_t init_read_secondary = inb(SBUS + ATA_REG_STATUS);
    
    if(init_read_primary == 0xFF) {
        #if DEBUG
        kprintf("Error: Primary Disk Not Connected, floating bus\n");
        #endif
    }
    if(init_read_secondary == 0xFF) {
        #if DEBUG
        kprintf("Error: Secondary Disk Not Connected, floating bus\n");
        #endif
    }
    if(init_read_primary == 0xFF && init_read_secondary == 0xFF) {
        return true;
    }
    return false;
}

static void resetBus(ATA_Drive* drive) {
    outb(drive->command_port, 1 << 2);
    outb(drive->command_port, 1 << 1);
    
}

static bool atapiSetup(ATA_Drive *drive){
    
    kprintf("ATAPI SETUP not implemented\n");
    return false;
    
    char* drive_str = (drive->type == PRIMARY) 
        ? "Primary" : "Secondary";
    
    uint16_t bus = drive->io_port;
    
    outb(bus + ATA_REG_DRIVESELECT, 0xA0);
    outb(bus + ATA_REG_SECCOUNT,    0x00);
    outb(bus + ATA_REG_LBALOW,      0x00);
    outb(bus + ATA_REG_LBAMID,      0x00);
    outb(bus + ATA_REG_LBAHIGH,     0x00);
    
    outb(bus + ATA_REG_STATUS, ATA_CMD_IDENTIFY_PACKET);
    
    uint8_t status = inb(bus + ATA_REG_STATUS);
    
    // Drive doesnt exist
    if(status == 0x00){
        kprintf("Error: %s drive does not exist\n", drive_str);
        return false;
    }
    
    ATA_Status_Register poll;
    inb_mem(bus + ATA_REG_STATUS, (uint8_t*)&poll);
    while(poll.busy) {
        inb_mem(bus + ATA_REG_STATUS, (uint8_t*)&poll);
    }
    
    inb_mem(bus + ATA_REG_STATUS, (uint8_t*)&poll);
    while(poll.data_transfer_requested != 1 && poll.error != 1){
        inb_mem(bus + ATA_REG_STATUS, (uint8_t*)&poll);
    }
    
    if(poll.error) {
        kprintf("Error: Bus %s is ATAPI, gave error status\n");
        return false;
    }
    
    uint16_t identify_data[256];
    kmemset(identify_data, 0, 256);
    for(int i = 0; i < 256; i++) {
        identify_data[i] = inw(bus);
    }
    
    uint32_t num_lba28_sectors = identify_data[60] | (identify_data[61] << 16);
    kprintf("num_lba28_sectors: %d\n", num_lba28_sectors);
    drive->num_sectors = num_lba28_sectors;
    
    return num_lba28_sectors > 0;
}

static void ataSetNoInterrupts(ATA_Drive* drive) {
    outb(drive->command_port, 0x02);
}

// Sends ATA IDENTIFY command
// Determines if type ATAPI, if so calls atapiSetup
static bool ataIdentify(ATA_Drive *drive) {
    char* drive_str = (drive->type == PRIMARY) 
        ? "Primary" : "Secondary";
    
    uint16_t bus = drive->io_port;
    uint16_t alt_status = drive->command_port;
    
    // Stop Interrupts, set nIEN in control register
    
    outb(bus + ATA_REG_DRIVESELECT, 0xA0);
    ataSetNoInterrupts(drive);
    outb(bus + ATA_REG_SECCOUNT,    0x00);
    outb(bus + ATA_REG_LBALOW,      0x00);
    outb(bus + ATA_REG_LBAMID,      0x00);
    outb(bus + ATA_REG_LBAHIGH,     0x00);
    
    // Send IDENTIFY Command
    outb(bus + ATA_REG_STATUS, ATA_CMD_IDENTIFY);
    
    uint8_t status = inb(bus + ATA_REG_STATUS);
    
    // Drive doesnt exist
    if(status == 0x00){
        kprintf("Error: %s drive does not exist\n", drive_str);
        return false;
    }
    
    
    
    // Wait for BSY flag to clear
    ATA_Status_Register poll;
    inb_mem(alt_status, (uint8_t*)&poll);
    while(poll.busy) {
        inb_mem(alt_status, (uint8_t*)&poll);
    }
    
    // Check if drive ATA, if not, probably ATAPI
    uint8_t lbamid = inb(bus + ATA_REG_LBAMID);
    uint8_t lbahigh = inb(bus + ATA_REG_LBAHIGH);
    
    if(lbamid == 0x14 && lbahigh == 0xEB){
        kprintf("%s bus is ATAPI\n", drive_str); 
        drive->ata_type = ATAPI;
        drive->uses_packets = true;
        return atapiSetup(drive);
    }
    kprintf("%s bus is ATA\n", drive_str); 
    
    inb_mem(alt_status, (uint8_t*)&poll);
    while(poll.data_transfer_requested != 1 && poll.error != 1){
        inb_mem(alt_status, (uint8_t*)&poll);
    }
    
    if(poll.error) {
        kprintf("Error: Bus %s is ATA, but gave status of error\n");
        return false;
    }
    
    uint16_t identify_data[256];
    kmemset(identify_data, 0, 256);
    for(int i = 0; i < 256; i++) {
        identify_data[i] = inw(bus);
    }
    
    uint16_t supports_lba_48_word = identify_data[83];
    if((supports_lba_48_word << 10) & 1){
        kprintf("%s supports lba48 mode\n", drive_str);
    }
    
    uint32_t num_lba28_sectors = identify_data[60] | (identify_data[61] << 16);
    kprintf("num_lba28_sectors: 0x%x\n", num_lba28_sectors);
    
    drive->num_sectors = num_lba28_sectors;
    
    if(num_lba28_sectors == 0)
        return false;
    
    return true;
}

static void idePoll(ATA_Drive* drive){
    ATA_Status_Register poll = {};
    inb_mem(drive->command_port, (uint8_t*)&poll);
    while(poll.busy != 0){
        inb_mem(drive->command_port, (uint8_t*)&poll);    
    }
}

int32_t __ataRead(ATA_Drive* drive, char* data, uint32_t num_sectors, uint32_t sector_num){
    if(drive->ata_type == ATAPI) {
        kprintf("ATAPI Writing not implemented\n");
        return 0;
    }
    
    if(sector_num > drive->num_sectors) {
        kprintf("ERROR: sector num too high for ATA\n");
        kprintf("    max: 0x%x\n", drive->num_sectors);
        return 0;
    }
    
    if(sector_num + num_sectors > drive->num_sectors) {
        kprintf("ERROR: ATA write aborted, would write off end of disk\n");
        kprintf("    max: 0x%x\n", drive->num_sectors);
        return 0;
    }
    
    uint32_t lba = (sector_num * SECTOR_SIZE) & 0x0FFFFFFF;
    uint8_t lba_high_ext = (uint8_t)(lba >> 24 & 0x0F);
    // select the drive
    uint8_t drive_select_and_high_bits = 0;
    if(drive->type == PRIMARY){
        drive_select_and_high_bits = ATA_PRIMARY_SELECT;
    }else {
        drive_select_and_high_bits = ATA_SECONDARY_SELECT;
    }
    
    drive_select_and_high_bits |= lba_high_ext;
    
    // NOTE(alex): Technically could take up to 30s for disk to spin up :(( 
    
    // Select drive and set highest 4 bits of lba
    outb(drive->io_port + ATA_REG_DRIVESELECT, drive_select_and_high_bits);
    
    // Waste time (wait 400ns)
    pitSleep(1);
    resetBus(drive);
    
    // Wait for BSY = 0 and data_transfer_requested == 0
    ATA_Status_Register poll = {};
    inb_mem(drive->command_port, (uint8_t*)&poll);
    while(poll.busy != 0 || poll.data_transfer_requested != 0){
        inb_mem(drive->command_port, (uint8_t*)&poll);    
    }
    
    uint8_t lba_low = lba & 0xFF;
    uint8_t lba_mid = (lba >> 8) & 0xFF;
    uint8_t lba_high = (lba >> 16) & 0xFF;
    
    // Write LBA, sector number, and sector count register
    outb(drive->io_port + ATA_REG_SECCOUNT, num_sectors);
    outb(drive->io_port + ATA_REG_LBALOW, lba_low);
    outb(drive->io_port + ATA_REG_LBAMID, lba_mid);
    outb(drive->io_port + ATA_REG_LBAHIGH, lba_high);
    
    // Read sectors command
    outb(drive->io_port + ATA_REG_COMMAND, ATA_CMD_READ_PIO);
    
    // Waste time (wait 400ns)
    pitSleep(1);
    
    
    // Wait for ready to transfer data
    inb_mem(drive->command_port, (uint8_t*)&poll);
    while(poll.busy != 0 || poll.data_transfer_requested != 1){
        inb_mem(drive->command_port, (uint8_t*)&poll);    
    }
    
    // Each sector size is 512bytes, so we will write 0 to any that we don't use at the end
    uint32_t i;
    uint16_t* itr = (uint16_t*)data;
    for(i = 0; i < num_sectors*(SECTOR_SIZE/2); i++){
        uint16_t tmp = inw(drive->io_port + ATA_REG_DATA);
        *itr = tmp;
        itr++;
    }
    
    // Cache Flush
    outb(drive->io_port + ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH);
    resetBus(drive);
    
    // Wait for ATA Interrupt
    inb_mem(drive->command_port, (uint8_t*)&poll);
    while(poll.busy != 0){
        inb_mem(drive->command_port, (uint8_t*)&poll);    
    }
    
    if(poll.error == 1) {
        kprintf("ERROR: Error writing to disk\n");
        return 0;
    }
    
    return num_sectors*SECTOR_SIZE;
}

int32_t ataRead(char* data, uint32_t num_sectors, uint32_t sector_num){
    return __ataRead(&ata_drives[0], data, num_sectors, sector_num);
}

static int32_t __ataWrite(ATA_Drive* drive, char* data, uint32_t num_sectors, uint32_t sector_num){
    if(drive->ata_type == ATAPI) {
        kprintf("ATAPI Writing not implemented\n");
        return 0;
    }
    
    if(sector_num > drive->num_sectors) {
        kprintf("ERROR: sector num too high for ATA\n");
        kprintf("    max: 0x%x\n", drive->num_sectors);
        return 0;
    }
    
    if(sector_num + num_sectors > drive->num_sectors) {
        kprintf("ERROR: ATA write aborted, would write off end of disk\n");
        kprintf("    max: 0x%x\n", drive->num_sectors);
        return 0;
    }
    
    uint32_t lba = (sector_num * SECTOR_SIZE) & 0x0FFFFFFF;
    uint8_t lba_high_ext = (uint8_t)(lba >> 24 & 0x0F);
    // select the drive
    uint8_t drive_select_and_high_bits = 0;
    if(drive->type == PRIMARY){
        drive_select_and_high_bits = ATA_PRIMARY_SELECT;
    }else {
        drive_select_and_high_bits = ATA_SECONDARY_SELECT;
    }
    
    drive_select_and_high_bits |= lba_high_ext;
    
    // NOTE(alex): Technically could take up to 30s for disk to spin up :(( 
    // Select drive and set highest 4 bits of lba
    outb(drive->io_port + ATA_REG_DRIVESELECT, drive_select_and_high_bits);
    
    // Waste time (wait 400ns)
    pitSleep(1);
    ataSetNoInterrupts(drive);
    
    ATA_Status_Register poll = {};
    inb_mem(drive->command_port, (uint8_t*)&poll);
    while(poll.busy != 0 || poll.data_transfer_requested != 0){
        inb_mem(drive->command_port, (uint8_t*)&poll);    
    }
    
    uint8_t lba_low = lba & 0xFF;
    uint8_t lba_mid = (lba >> 8) & 0xFF;
    uint8_t lba_high = (lba >> 16) & 0xFF;
    
    // Write LBA, sector number, and sector count register
    outb(drive->io_port + ATA_REG_SECCOUNT, num_sectors);
    outb(drive->io_port + ATA_REG_LBALOW, lba_low);
    outb(drive->io_port + ATA_REG_LBAMID, lba_mid);
    outb(drive->io_port + ATA_REG_LBAHIGH, lba_high);
    
    // Write sectors command
    outb(drive->io_port + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);
    
    // Waste time (wait 400ns)
    pitSleep(1);
    
    
    // Wait for ready to transfer data
    inb_mem(drive->command_port, (uint8_t*)&poll);
    while(poll.busy != 0 || poll.data_transfer_requested != 1){
        inb_mem(drive->command_port, (uint8_t*)&poll);    
    }
    
    // Each sector size is 512bytes, so we will write 0 to any that we don't use at the end
    uint32_t i;
    for(i = 0; i < num_sectors*(SECTOR_SIZE/2); i++){
        uint16_t to_write = *data;
        data++;
        to_write |= (*(uint16_t*)data<<8);
        data++;
        outw(drive->io_port + ATA_REG_DATA, to_write);
    }
    
    // Cache Flush
    outb(drive->io_port + ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH);
    resetBus(drive);
    
    // Wait for ATA Interrupt
    inb_mem(drive->command_port, (uint8_t*)&poll);
    while(true){
        if(poll.data_transfer_requested == 0 && poll.busy == 0)
            break;
        inb_mem(drive->command_port, (uint8_t*)&poll);    
    }
    
    if(poll.error == 1) {
        kprintf("ERROR: Error writing to disk\n");
        return 0;
    }
    
    return num_sectors*SECTOR_SIZE;
}

int32_t ataWrite(char* data, uint32_t num_sectors, uint32_t sector_num){
    return __ataWrite(&ata_drives[0], data, num_sectors, sector_num);
}

void ideIRQHandler() {
    interrupt_recieved = true;
    kprintf("written!!!!!!!!!!!!!!!!!!!!!\n");
}

bool ideInit() {
    if(isFloatingBus()) {
        kprintf("Error: Floating Buses, No Drives Connected\n");
        return false;   
    }
    
    ATA_Drive primary_drive;
    primary_drive.ata_type = ATA;
    primary_drive.type = PRIMARY;
    primary_drive.io_port = PBUS;
    primary_drive.command_port = PBUSC;
    
    
    ATA_Drive secondary_drive;
    secondary_drive.ata_type =  ATA;
    secondary_drive.type = SECONDARY;
    secondary_drive.io_port = SBUS;
    secondary_drive.command_port = PBUSC;
    
    ata_drives[0] = primary_drive;
    ata_drives[1] = secondary_drive;
    
    // IDENTIFY
    if(ataIdentify(&primary_drive)) {
        kprintf("Primary Bus is setup and read to read/write\n");
    }
    if(ataIdentify(&secondary_drive)) {
        kprintf("Secondary Bus is setup and ready to read/write\n");
    }else{
        kprintf("Secondary Bus is NOT setup\n");
    }
    
    return true;
}
    
    