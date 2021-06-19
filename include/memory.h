#pragma once

typedef uint32_t PageDirEntry;

typedef struct {
    uint32_t flags;     // 0
    uint32_t mem_lower; // 4
    uint32_t mem_upper; // 8
    
    // not used by us
    uint32_t boot_device; // 12
    uint32_t cmdline;     // 16
    uint32_t mods_count;  // 20
    uint32_t mods_addr;   // 24
    uint32_t syms_1;      // 28
    uint32_t syms_2;      // 32
    uint32_t syms_3;      // 36
    uint32_t syms_4;      // 40

    // memory map
    uint32_t mmap_length; // 44
    uint32_t mmap_addr;   // 48

    // plus a buch more fields that we're not using...
} __attribute__((packed)) MultibootInfo;

void loadPhysicalMemoryRegionDescriptors(MultibootInfo* multiboot_info);
void setupPaging();
