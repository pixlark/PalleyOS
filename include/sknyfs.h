#pragma once

/* SknyFS filesystem driver
 *
 *
 */

typedef struct {
    uint8_t drive;
} SknyHandle;

typedef enum {
    SKNY_STATUS_OK,
    SKNY_WRITE_FAILURE,
    SKNY_READ_FAILURE,
    SKNY_FILESYSTEM_FULL,
    SKNY_FILE_NOT_FOUND
} SknyStatus;

extern const char* sknyStatusToString(SknyStatus status);

SknyStatus sknyCreateFilesystem(SknyHandle* handle, uint8_t drive);
SknyStatus sknyCreateFile(SknyHandle* handle, const char* name);
