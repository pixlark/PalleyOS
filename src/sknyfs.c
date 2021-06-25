/* 
 * SknyFS filesystem driver
 */

#include <stdbool.h>

#include <ata.h>
#include <kstdio.h>
#include <sknyfs.h>

// These types make it more clear what numbers indicate what.
typedef uint32_t ChunkLocation;    // Indexes disk by chunk
typedef uint32_t SectorLocation;   // Indexes disk by sector
typedef uint32_t AbsoluteLocation; // Indexes disk by byte

typedef uint32_t FileIndex; // Indexes file map

// Must be a multiple of ATA_SECTOR_SIZE! (512 bytes)
// Must be a multiple of sizeof(FileMetadata)! (256 bytes)
//#define CHUNK_SIZE (16 * 1024 * 1024)
#define CHUNK_SIZE 1024

#define SECTORS_PER_CHUNK (CHUNK_SIZE / ATA_SECTOR_SIZE)

#define CHUNKS_IN_ALLOCATION_MAP (16)
#define CHUNKS_IN_FILE_MAP       (16)

typedef struct {
    // An empty name '' indicates that this file metadata does not exist
    uint8_t name[252];
    ChunkLocation location;
} __attribute__((packed)) FileMetadata;

#define ALLOCATION_MAP_BEGIN (0)
#define FILE_MAP_BEGIN (CHUNKS_IN_ALLOCATION_MAP * CHUNK_SIZE)
#define STORAGE_BEGIN ((CHUNKS_IN_ALLOCATION_MAP * CHUNKS_IN_FILE_MAP) * CHUNK_SIZE)

#define FILES_PER_CHUNK (CHUNK_SIZE / sizeof(FileMetadata))

#define TOTAL_ALLOCATABLE_CHUNKS \
    (CHUNKS_IN_ALLOCATION_MAP * CHUNK_SIZE * 8)
#define MAXIMUM_FILE_COUNT \
    (CHUNKS_IN_FILE_MAP * CHUNK_SIZE / sizeof(FileMetadata))

#define INFORMATION_DUMP true

static const char* sknyStatusStrings[] = {
    "SKNY_STATUS_OK",
    "SKNY_WRITE_FAILURE",
    "SKNY_READ_FAILURE",
    "SKNY_FILESYSTEM_FULL"
};

const char* sknyStatusToString(SknyStatus status) {
    return sknyStatusStrings[status];
}

static SknyStatus searchAllocationMap(SknyHandle* handle, ChunkLocation* ret) {
    for (uint32_t i = 0; i < CHUNKS_IN_ALLOCATION_MAP; i++) {
        uint8_t map_chunk[CHUNK_SIZE];
        AbsoluteLocation read_location = ALLOCATION_MAP_BEGIN + (i * CHUNK_SIZE);
        uint8_t err = ideReadSectors(handle->drive, SECTORS_PER_CHUNK, read_location, map_chunk);
        if (err != 0) {
            ide_print_error(handle->drive, err);
            return SKNY_READ_FAILURE;
        }
        for (uint32_t j = 0; j < CHUNK_SIZE; j++) {
            uint8_t byte_field = map_chunk[j];
            if (byte_field != 0xff) {
                // Found a chunk!
                for (uint8_t bit = 0; bit < 8; bit++) {
                    if ((byte_field & (1 << bit)) == 0) {
                        ChunkLocation location = 0;
                        location += i * (CHUNK_SIZE * 8); // Offset by bits per allocation map chunk
                        location += j * 8;                // Ofset by bits per byte in allocation chunk
                        location += bit;                  // Offset by bit in this byte
                        *ret = location;
                        return SKNY_STATUS_OK;
                    }
                }
            }
        }
    }
    return SKNY_FILESYSTEM_FULL;
}

// TODO(Paul): If this function reads in only the sector containing
// the bit, and not the entire chunk containing the bit, it will
// become way, way, way more efficient. So do that.
static SknyStatus markChunkAsUsed(SknyHandle* handle, ChunkLocation chunk_number) {
    const uint32_t BITS_PER_CHUNK        = CHUNK_SIZE * 8;
    uint32_t       map_chunk             = chunk_number / BITS_PER_CHUNK;
    uint32_t       offset_into_map_chunk = chunk_number % BITS_PER_CHUNK;
    uint32_t       byte_offset           = offset_into_map_chunk / 8;
    uint8_t        bit_offset            = offset_into_map_chunk % 8;
    // Read in chunk
    uint8_t map_chunk_buffer[CHUNK_SIZE];
    /*
    for (int i = 0; i < CHUNK_SIZE; i++) {
        kprintf("%x,", map_chunk_buffer[i]);
    }
    kprintf("\n");*/
    AbsoluteLocation map_chunk_location = ALLOCATION_MAP_BEGIN + (map_chunk * CHUNK_SIZE);
    uint8_t err = ideReadSectors(handle->drive, SECTORS_PER_CHUNK, map_chunk_location, map_chunk_buffer);
    if (err != 0) {
        ide_print_error(handle->drive, err);
        return SKNY_READ_FAILURE;
    }
    // Set the bit
    map_chunk_buffer[byte_offset] |= (1 << bit_offset);
    // Write the chunk back
    err = ideWriteSectors(handle->drive, SECTORS_PER_CHUNK, map_chunk_location, map_chunk_buffer);
    if (err != 0) {
        ide_print_error(handle->drive, err);
        return SKNY_WRITE_FAILURE;
    }
    return SKNY_STATUS_OK;
}

static SknyStatus searchFileMap(SknyHandle* handle, FileIndex* ret) {
    for (ChunkLocation chunk_index = 0; chunk_index < CHUNKS_IN_FILE_MAP; chunk_index++) {
        FileMetadata files[FILES_PER_CHUNK];
        AbsoluteLocation location = FILE_MAP_BEGIN + (chunk_index * CHUNK_SIZE);
        uint8_t err = ideReadSectors(handle->drive, SECTORS_PER_CHUNK, location, (uint8_t*) files);
        if (err != 0) {
            ide_print_error(handle->drive, err);
            return SKNY_READ_FAILURE;
        }
        for (uint32_t file_index; file_index < FILES_PER_CHUNK; file_index++) {
            FileMetadata file = files[file_index];
            kprintf("file %u: name (%s) location (%u)\n", (chunk_index * FILES_PER_CHUNK) + file_index, file.name, file.location);
            if (file.name[0] == '\0') {
                // We've found an empty spot!
                *ret = (chunk_index * FILES_PER_CHUNK) + file_index;
                return SKNY_STATUS_OK;
            }
        }
    }
    return SKNY_FILESYSTEM_FULL;
}

// TODO(Paul): If this function reads in only the sector containing
// the metadata, and not the entire chunk containing the metadata, it
// will become a bit more efficient. So do that.
static writeFileMetadata(SknyHandle* handle, FileIndex file_index, FileMetadata* file_metadata) {
    ChunkLocation chunk_offset = file_index / FILES_PER_CHUNK;
    AbsoluteLocation location = FILE_MAP_BEGIN + (chunk_offset * CHUNK_SIZE);
    FileMetadata files[FILES_PER_CHUNK];
    uint8_t err = ideReadSectors(handle->drive, SECTORS_PER_CHUNK, location, (uint8_t*) files);
    if (err != 0) {
        ide_print_error(handle->drive, err);
        return SKNY_READ_FAILURE;
    }
    files[file_index % FILES_PER_CHUNK] = *file_metadata;
    err = ideWriteSectors(handle->drive, SECTORS_PER_CHUNK, location, (uint8_t*) files);
    if (err != 0) {
        ide_print_error(handle->drive, err);
        return SKNY_WRITE_FAILURE;
    }
    return SKNY_STATUS_OK;
}

SknyStatus sknyCreateFile(SknyHandle* handle, const char* name) {
    //
    // First, can we fit the file?
    //
    ChunkLocation available_chunk;
    SknyStatus status = searchAllocationMap(handle, &available_chunk);
    if (status != SKNY_STATUS_OK) {
        return status;
    }
    #if INFORMATION_DUMP
    kprintf("ALLOCATING FILE TO CHUNK %u\n", available_chunk);
    #endif

    FileIndex file_index;
    status = searchFileMap(handle, &file_index);
    if (status != SKNY_STATUS_OK) {
        return status;
    }

    #if INFORMATION_DUMP
    kprintf("REPRESENTING FILE WITH BIN %u\n", file_index);
    #endif

    //
    // Now, actually create the file
    //
    markChunkAsUsed(handle, available_chunk);

    // Somehow doing *just* this changes the file map? This is an issue.
    
    //FileMetadata file_metadata;
    //kstrncpy(file_metadata.name, name, 252);
    //file_metadata.location = available_chunk;
    //writeFileMetadata(handle, file_index, &file_metadata);
    
    return SKNY_STATUS_OK;
}

// Very slow!!
static uint8_t clearChunk(SknyHandle* handle, ChunkLocation chunk, uint8_t byte) {
    uint8_t buffer[ATA_SECTOR_SIZE];
    // TODO(Paul): get memset from alex!!
    for (uint32_t i = 0; i < ATA_SECTOR_SIZE; i++) {
        buffer[i] = byte;
    }
    for (uint32_t i = 0; i < SECTORS_PER_CHUNK; i++) {
        uint8_t err = ideWriteSectors(
            handle->drive, 1,
            (chunk * CHUNK_SIZE) + (i * ATA_SECTOR_SIZE),
            buffer
        );
        if (err != 0) {
            return err;
        }
    }
    return 0;
}

SknyStatus sknyCreateFilesystem(SknyHandle* handle, uint8_t drive) {
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
    kprintf("(!!!)   Formatting file map...\n");
    #endif

    // Initialize file map
    for (ChunkLocation i = 0; i < CHUNKS_IN_FILE_MAP; i++) {
        uint8_t write_err = clearChunk(handle, CHUNKS_IN_ALLOCATION_MAP + i, 0);
        if (write_err != 0) {
            ide_print_error(handle->drive, write_err);
            return SKNY_WRITE_FAILURE;
        }
    }

    #if INFORMATION_DUMP
    kprintf("(!!!) DONE FORMATTING\n");
    kprintf("=======================================\n");
    #endif

    return SKNY_STATUS_OK;
}
