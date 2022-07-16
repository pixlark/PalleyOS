#include <stdbool.h>
#include <stdint.h>
#include <memory.h>
#include <kstdio.h>

const uint32_t FRAME_SIZE = 4 * 1024 * 1024;

// The Page Directory maps sections of the virtual address space into
// equivalent sections of the physical address space.
static PageDirEntry page_directory[1024] __attribute__((aligned(4096)));

typedef enum {
    PAGE_SIZE_4_KIB,
    PAGE_SIZE_4_MIB
} PageSize;

typedef enum {
    PAGE_READ_ONLY,
    PAGE_READ_WRITE
} PageRW;

typedef enum {
    PAGE_SUPERVISOR_ONLY,
    PAGE_USER_AND_SUPERVISOR
} PagePermission;

static PageDirEntry constructPageDirEntry(
                                          uint32_t pfn,
                                          PageSize page_size,
                                          PageRW rw,
                                          PagePermission permission
                                          ) {
    PageDirEntry entry = 0;
    // Sanity check (slow, but we're new and bugs are scary)
    if (pfn >= 0x40000) {
        kprintf("constructPageDirEntry received a bad PFN!\n");
        while (true);
    }
    entry |= pfn << 22; // bits 31:22
    entry |= (page_size == PAGE_SIZE_4_MIB)
        ? (1 << 7) : 0;
    entry |= (rw == PAGE_READ_WRITE)
        ? (1 << 1) : 0;
    entry |= (permission == PAGE_USER_AND_SUPERVISOR)
        ? (1 << 2) : 0;
    entry |= 1; // Mark present
    return entry;
}

static PageDirEntry constructEmptyPageDirEntry(
                                               PageSize page_size,
                                               PageRW rw,
                                               PagePermission permission
                                               ) {
    PageDirEntry entry =
        constructPageDirEntry(0, page_size, rw, permission);
    // Clear present bit
    entry &= ~1;
    return entry;
}


// This is a bitfield that keeps track of which page frames are in
// use, versus which ones are still free. This DOES NOT keep track of
// any parts of physical memory that are completely unavailable
// (i.e. not backed up by actual RAM). That is managed by the
// Physical_Memory_Regions (see below). Those areas of memory are just
// marked permanently free
static union {
    uint8_t  bytes[128];
    uint16_t halfwords[64];
    uint32_t words[32];
} page_frame_map;

// Virtual Page Number: VPN
// This is the part of the address that indexes the page directory.
// The page directory has 1024 entries, so the top 10 bits are the VPN.
static uint32_t getVirtualFrameNumber(uint32_t virtual_address) {
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
typedef struct {
    uint32_t address;
    uint32_t size;
} Physical_Memory_Region;

#define MAX_PHYSICAL_MEMORY_REGIONS 16
static Physical_Memory_Region physical_memory_regions[MAX_PHYSICAL_MEMORY_REGIONS];
static uint32_t physical_memory_region_count;

static bool is_frame_used(uint32_t pfn) {
    return page_frame_map.words[pfn / 32] & (1 << (pfn % 32));
}

static void setFrameUsed(uint32_t pfn) {
    page_frame_map.words[pfn / 32] |= (1 << (pfn % 32));
}

extern void flush_tlb(); // in boot.s for now

// When a page fault occurs, this is called. It finds a frame of
// physical memory that is so far unused, and maps the page directory
// entry corresponding to the fault to this frame. It also marks that
// frame as used, so it doesn't get reused in the future.
extern uint32_t getFaultAddress(); // in boot.s for now
void handle_page_fault() {
    uint32_t fault_address = getFaultAddress();
    uint32_t pfn;
    // Find an unallocated page frame
    for (uint8_t region_index = 0; region_index < physical_memory_region_count; region_index++) {
        Physical_Memory_Region* region = physical_memory_regions + region_index;
        
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
        //kprintf("frame start: %x; frame count: %d\n", frame_start, frame_count);
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
    setFrameUsed(pfn);
    
    // Now, update the page directory to reference this frame
    uint32_t vpn = getVirtualFrameNumber(fault_address);
	page_directory[vpn] = constructPageDirEntry(
                                                pfn, PAGE_SIZE_4_MIB, PAGE_READ_WRITE, PAGE_USER_AND_SUPERVISOR
                                                );
    
    // Invalidate the TLB, so that the processor will reflect these changes
    flush_tlb(); 
    
    //kprintf("Mapped page %d to frame %d (word %d, position %d)\n", vpn, pfn, pfn / 32, pfn % 32);
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
void loadPhysicalMemoryRegionDescriptors(MultibootInfo* multiboot_info) {
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
            physical_memory_regions[physical_memory_region_count] = (Physical_Memory_Region)
            { region->base_addr_low, region->length_low };
            physical_memory_region_count += 1;
        }
        region = (PhysicalMemoryRegionDescriptor*)
        (((uint8_t*) region) + region->size + sizeof(region->size));
    }
    kprintf("====================\n");
}

extern void enablePaging(void*); // in boot.s for now

// Setting this bit allows us to use huge pages that span 4 MiB of
// virtual address space, rather than cascading from the page
// directory into smaller page tables that index 4 KiB of virtual
// address space.
const uint32_t PS_BIT = (1<<7);

void setupPaging() {
    //  Zero the page directory entirely
    //  We're setting proper bits on these unused entries, but we
    //  really don't need to. The OS will never look at an entry until
    //  it's marked present.
    for (int i = 0; i < 1024; i++) {
        page_directory[i] = 0;
        /*
        page_directory[i] = constructEmptyPageDirEntry(
            PAGE_SIZE_4_MIB,
            PAGE_READ_WRITE,
            PAGE_USER_AND_SUPERVISOR
            );*/
    }
    
    // Map low memory (first 4MB) (this is what we're operating under)
    page_directory[0] = constructPageDirEntry(
                                              0, PAGE_SIZE_4_MIB, PAGE_READ_WRITE, PAGE_USER_AND_SUPERVISOR
                                              );
    
    // Zero out our physical page frame map (no pages allocated)
    for (int i = 0; i < 32; i++) {
        page_frame_map.words[i] = 0;
    }
    
    // Except for low memory, again
    page_frame_map.words[0] |= (1 << 0);
    
    // Finally we can actually enable paging
	enablePaging(page_directory);
}
