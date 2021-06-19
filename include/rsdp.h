#pragma once
#include <acpi.h>


// Searches through main BIOS memory ( < 1 MiB )
// for the rsdp, assumes not using UEFI
// Returns NULL on failure
struct RSDPDescriptor* get_rsdp();
