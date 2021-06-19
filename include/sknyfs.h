#pragma once

/* SknyFS filesystem driver
 *
 *
 */

typedef struct {
    uint8_t drive;
} SknyHandle;

typedef enum {
    SKNY_CREATE_OK,
    SKNY_WRITE_FAILURE,
} CreateStatus;

typedef uint32_t ChunkLocation;

CreateStatus createFilesystem(SknyHandle* handle, uint8_t drive);
