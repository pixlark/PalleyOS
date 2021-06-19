/* SknyFS filesystem driver
 *
 *
 */

#include <stdbool.h>

#include <tio.h>

#include <ata.h>
#include <sknyfs.h>

// Must be a multiple of ATA_SECTOR_SIZE! (512 bytes)
#define CHUNK_SIZE (16 * 1024 * 1024)

#define SECTORS_PER_CHUNK (CHUNK_SIZE / ATA_SECTOR_SIZE)

#define CHUNKS_IN_ALLOCATION_MAP (16)
#define CHUNKS_IN_FILE_MAP       (16)

typedef struct {
    // An empty name '' indicates that this file metadata does not exist
    uint8_t name[252];
    uint32_t location;
} __attribute__((packed)) FileMetadata;

#define FILES_PER_CHUNK (CHUNK_SIZE / sizeof(FileMetadata));

#define TOTAL_ALLOCATABLE_CHUNKS \
    (CHUNKS_IN_ALLOCATION_MAP * CHUNK_SIZE * 8)
#define MAXIMUM_FILE_COUNT \
    (CHUNKS_IN_FILE_MAP * CHUNK_SIZE / sizeof(FileMetadata))

#define INFORMATION_DUMP true

// Very slow!!
uint8_t clearChunk(SknyHandle* handle, ChunkLocation chunk, uint8_t byte) {
    char buffer[ATA_SECTOR_SIZE];
    // TODO(Paul): get memset from alex!!
    for (uint32_t i = 0; i < ATA_SECTOR_SIZE; i++) {
        buffer[i] = byte;
    }
    for (uint32_t i = 0; i < SECTORS_PER_CHUNK; i++) {
        uint8_t err = ide_write_sectors(handle->drive, 1, chunk * CHUNK_SIZE, buffer);
        if (err != 0) {
            return err;
        }
    }
    return 0;
}

CreateStatus createFilesystem(SknyHandle* handle, uint8_t drive) {
    handle->drive = drive;
    
    #if INFORMATION_DUMP
    kprintf("=== INITIALIZING Skny FILESYSTEM... ===\n");
    kprintf("Chunk size: %u bytes\n", CHUNK_SIZE);
    kprintf("Allocation map: %u chunks\n", CHUNKS_IN_ALLOCATION_MAP);
    kprintf("File map: %u chunks\n", CHUNKS_IN_FILE_MAP);
    kprintf("File metadata size: %u bytes\n", sizeof(FileMetadata));
    kprintf("Total allocatable chunks: %u chunks\n", TOTAL_ALLOCATABLE_CHUNKS);
    kprintf("Maximum file count: %u files\n", MAXIMUM_FILE_COUNT);

    kprintf("(!!!) FORMATTING TO Skny...\n");
    kprintf("(!!!)   Formatting allocation map...\n");
    #endif
    
    // Initialize allocation map
    for (ChunkLocation i = 0; i < CHUNKS_IN_ALLOCATION_MAP; i++) {
        uint8_t write_err = clearChunk(handle, i, 0);
        if (write_err != 0) {
            ide_print_error(handle->drive, write_err);
            return SKNY_WRITE_FAILURE;
        }
    }
    
    #if INFORMATION_DUMP
    kprintf("(!!!)   Formatting allocation map...\n");
    #endif

    // Initialize file map
    for (ChunkLocation i = 0; i < CHUNKS_IN_ALLOCATION_MAP; i++) {
        
    }

    #if INFORMATION_DUMP
    kprintf("=======================================\n");
    #endif


    return SKNY_CREATE_OK;
}
