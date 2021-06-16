#include <stdbool.h>
#include <stdint.h>
#include <memory.h>
#include <kstdio.h>

const uint32_t FRAME_SIZE = 4 * 1024 * 1024;

// The Page Directory maps sections of the virtual address space into
// equivalent sections of the physical address space.
static PageDirEntry page_directory[1024] __attribute__((aligned(4096)));

// This is a bitfield that keeps track of which page frames are in
// use, versus which ones are still free. This DOES NOT keep track of
// any parts of physical memory that are completely unavailable
// (i.e. not backed up by actual RAM). That is managed by the
// PhysicalMemoryRegions (see below). Those areas of memory are just
// marked permanently free
static union {
    uint8_t  bytes[128];
    uint16_t halfwords[64];
    uint32_t words[32];
} page_frame_map;

// Virtual Page Number: VPN
//  This is the part of the address that indexes the page directory.
//  The page directory has 1024 entries, so the top 10 bits are the VPN.
static uint32_t get_virtual_page_number(uint32_t virtual_address) {
    return virtual_address >> 22;
}

// Physical Frame Number: PFN
//  This is the part of the address that indexes physical memory by frame.
//  We divide memory into 4 MiB frames, each of which is indexed by the lower 22 bits.
//  Therefore, the top 10 bits are the PFN.
static uint32_t get_physical_frame_number(uint32_t physical_address) {
    return physical_address >> 22;
}

// The PhysicalMemoryRegions describe which parts of the 32-bit
// physical address space are mapped to actual RAM
typedef struct PhysicalMemoryRegion {
    uint32_t address;
    uint32_t size;
} PhysicalMemoryRegion;

#define MAX_PHYSICAL_MEMORY_REGIONS 16
static PhysicalMemoryRegion physical_memory_regions[MAX_PHYSICAL_MEMORY_REGIONS];
static uint32_t physical_memory_region_count;

static bool is_frame_used(uint32_t pfn) {
    return page_frame_map.words[pfn / 32] & (1 << (pfn % 32));
}

static void set_frame_used(uint32_t pfn) {
    page_frame_map.words[pfn / 32] |= (1 << (pfn % 32));
}

extern void flush_tlb(); // in boot.s for now

// When a page fault occurs, this is called. It finds a frame of
// physical memory that is so far unused, and maps the page directory
// entry corresponding to the fault to this frame. It also marks that
// frame as used, so it doesn't get reused in the future.
extern uint32_t get_fault_address(); // in boot.s for now
void handle_page_fault() {
    uint32_t fault_address = get_fault_address();
    uint32_t pfn;
    // Find an unallocated page frame
    for (uint8_t region_index = 0; region_index < physical_memory_region_count; region_index++) {
        PhysicalMemoryRegion* region = physical_memory_regions + region_index;
        
        // Frames must be aligned to the frame size, and there must be
        // enough space for the whole frame. Thus we calculate an
        // offset from the start of the region, and the maximum number
        // of blocks that it can fit.

        uint32_t frame_start = region->address & ~(FRAME_SIZE - 1);
        frame_start += FRAME_SIZE;

        if (frame_start > region->address + region->size) {
            // This region isn't big enough for even a single frame!
            // Low memory (kernel memory) should fall into this category
            continue;
        }
        
        uint32_t remaining_size = region->size - (frame_start - region->address);
        
        uint8_t frame_count = remaining_size / FRAME_SIZE;
        kprintf("frame start: %x; frame count: %d\n", frame_start, frame_count);
        for (uint32_t frame = 0; frame < frame_count; frame++) {
            uint32_t address = frame_start + frame * FRAME_SIZE;
            pfn = get_physical_frame_number(address);
            if (!is_frame_used(pfn)) {
                goto found_id;
            }
        }
    }
    // Could not allocate page frame
    kprintf("Ran out of physical memory!");
    while (1);

 found_id:
    // Set frame as allocated
    set_frame_used(pfn);

    // Now, update the page directory to reference this frame
    uint32_t vpn = get_virtual_page_number(fault_address);
	page_directory[vpn] |= (1 << 7); 	// Set Page Size (4MiB)
    //page_directory[vpn] |= (pfn << 22); // Set PFN
    page_directory[vpn] |= (1 << 22); // Set PFN
    page_directory[vpn] |= (3 << 5); // Set PFN
    page_directory[vpn] |= 3;           // Mark page as present

    // Invalidate the TLB, so that the processor will reflect these changes
    flush_tlb(); 

    kprintf("Mapped page %d to frame %d (word %d, position %d)\n", vpn, pfn, pfn / 32, pfn % 32);
}

typedef struct {
    uint32_t size; // Size of this region descriptor, minus the size
                   // of 'size' itself; just used for iteration
    uint32_t base_addr_low;
    uint32_t base_addr_high; // these high bits are useless to us running on 32-bit
    uint32_t length_low;
    uint32_t length_high;    // ^
    uint32_t region_type;
} __attribute__((packed)) PhysicalMemoryRegionDescriptor;

// Use the information GRUB passes us after bootup to figure out what
// parts of the physical address space are actually backed by real
// memory.
void load_physical_memory_region_descriptors(MultibootInfo* multiboot_info) {
    if ((multiboot_info->flags & (1 << 6)) == 0) {
        kprintf("GRUB did not provide memory information. Failure.\n");
        while (1);
    }

    PhysicalMemoryRegionDescriptor* region = (PhysicalMemoryRegionDescriptor*) multiboot_info->mmap_addr;
    uint32_t traversed_size = 0;
    kprintf("==== MEMORY MAP ====\n");
    physical_memory_region_count = 0;
    while (traversed_size < multiboot_info->mmap_length) {
        traversed_size += region->size + sizeof(region->size);
        kprintf("%s from: %x to %x\n",
            region->region_type == 1 ? "RAM   " : "No RAM",
            region->base_addr_low,
            region->base_addr_low + region->length_low
        );
        if (region->region_type == 1) {
            physical_memory_regions[physical_memory_region_count] = (PhysicalMemoryRegion)
                { region->base_addr_low, region->length_low };
            physical_memory_region_count += 1;
        }
        region = (PhysicalMemoryRegionDescriptor*)
            (((uint8_t*) region) + region->size + sizeof(region->size));
    }
    kprintf("====================\n");
}

extern void enable_paging(void*); // in boot.s for now

// Setting this bit allows us to use huge pages that span 4 MiB of
// virtual address space, rather than cascading from the page
// directory into smaller page tables that index 4 KiB of virtual
// address space.
const uint32_t PS_BIT = (1<<7);

void setup_paging() {
    // Zero the page directory entirely
    for (int i = 0; i < 1024; i++) {
        PageDirEntry entry = 0;
        entry |= PS_BIT; // 4 MiB page size
        entry |=  0b10;  // Read/Write
        entry |= 0b100;  // User/Supervisor
        page_directory[i] = entry;
    }

    // Map low memory (first 4MB) (this is what we're operating under)
    page_directory[0] |= 0b11;

    // Zero out our physical page frame map (no pages allocated)
    for (int i = 0; i < 32; i++) {
        page_frame_map.words[i] = 0;
    }

    // Except for low memory, again
    page_frame_map.words[0] |= (1 << 0);

    // Finally we can actually enable paging
	enable_paging(page_directory);
}
