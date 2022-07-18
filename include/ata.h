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
#include <stdbool.h>

#define IDE_SECTOR_SIZE 512

bool ideInit ();
void ideIRQHandler();
uint32_t ideWrite(char* data, uint32_t num_sectors, uint32_t sector_num);

