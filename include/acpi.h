#pragma once
#include <stddef.h>
#include <stdint.h>

/* ===== Root System Description Pointer ===== */
// (RSDP) Root System Description Pointer v 1.0 (2.0>= is for x64)
// This is used in the ACPI Programming Interface
// This gives us the RSDT -> FADT
struct RSDPDescriptor {
	char signature[8];
	uint8_t checksum;
	char OEMID[6];
	uint8_t revision;
	uint32_t rsdt_address; // $$ Money $$
} __attribute__((__packed__));

/* ===== System Descripton Table ===== */
// There are many types of SDT's, the 1st part is this header,
// which is common to all SDT's
struct ACPISDTHeader {
	char signature[4]; // 4 character unique ID, e.g. "FACP"
	uint32_t length;   // Total size of the table, inclusive of the header
	uint8_t revision;
	uint8_t checksum;
	char OEMID[6];
	char oem_table_id[8];
	uint32_t oem_revision;
	uint32_t creator_id;
	uint32_t creator_revision;
};

/* ===== Root System Description Table ===== */
// This contains pointers to all the other SDT's
struct RSDT {
	struct ACPISDTHeader header;

	// header.length gives total size of the table, inclusive with header
	// We need to subtract off sizeof(header) to get sizeof(table)
	// Divide by 4 because each pointer is 4 bytes long (32-bit)
	// num entries: (header.length - sizeof(header)) / 4
	uint32_t ptrs_to_other_sdts[];
};

/* ===== Fixed ACPI Description Table (FADT) ===== */
// Is pointed to by an entry in the RSDT. the signature is "FACP"

// Not exactly sure if this is applicable to us...
struct GenericAddressStructure {
	uint8_t address_space;
	uint8_t bit_width;
	uint8_t bit_offset;
	uint8_t access_size;
	
	// This is meant to be a uin64_t, so I assume that we
	// will never actually use this structure.
	uint32_t addr_high;
	uint32_t addr_low;
};

struct FADT {
	struct ACPISDTHeader header;
	uint32_t firmware_ctl;  // Pointer to FACS
	uint32_t dsdt;			// $$ Pointer to the DSDT $$

	// Field used in ACPI 1.0; no longer in use
	uint8_t reserved;

	uint8_t ppmp; // Preferred power management profile

	// System Control Interrupt, used by the ACPI to notify
	// the OS about fixed events, e.g. pressing of Power Button,
	// or General Purpose Events (GPEs), firmware specific.
	// This indicated the PIC or IOAPIC interrupt pin for it.
	uint16_t sci_interrupt;
	// An I/O port. This is where the OS writes ACPI Enable or Disable
	// to get or release the ownership over to the ACPI registers.
	// This is 0 on system s where System Management Mode (SMM) is not supported.
	uint32_t smi_command_port;
	uint8_t acpi_enable;
	uint8_t acpi_disable;

	uint8_t s4_bios_req;
	uint8_t pstate_control;
	uint32_t pm1a_event_block;
	uint32_t pm1b_event_block;
	uint32_t pm1a_control_block;
	uint32_t pm1b_control_block;
	uint32_t pm2_control_block;
	uint32_t pmt_timer_block;
	uint32_t gpe0_block;
	uint32_t gpe1_block;
	uint8_t pm1_event_length;
	uint8_t pm2_control_length;
	uint8_t pmt_timer_length;
	uint8_t gpe0_length;
	uint8_t gpe1_length;
	uint8_t gpe1_base;
	uint8_t cstate_control;
	uint16_t worst_c2_latency;
	uint16_t worst_c3_latency;
	uint16_t flush_size;
	uint16_t flush_stride;
	uint8_t duty_offset;
	uint8_t duty_width;
	uint8_t day_alarm;
	uint8_t month_alarm;
	uint8_t century;

	// Reserved in ACPI 1.0; used since ACPI 2.0+
	uint16_t boot_arch_flags; // Boot Architecture Flags

	uint8_t reserved2;
	uint32_t flags;

	struct GenericAddressStructure reset_reg;

	uint8_t reset_value;
	uint8_t reserved3[3];

	// There is some other stuff here in the extended version,
	// but it is 64-bit stuff, so I assume that we won't/can't use it.
};


