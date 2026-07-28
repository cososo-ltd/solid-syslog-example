#ifndef SEMIHOSTINGDISK_H
#define SEMIHOSTINGDISK_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /* Host-backed flat-disk media over ARM semihosting (BKPT 0xAB), under the
     * FatFs disk_* glue in diskio.c. Every run starts with the image absent, so
     * EnsureReady creates a fresh 8 MiB sparse file and the mount then sees a
     * zero-filled image and formats it.
     *
     * 16384 sectors x 512 B = 8 MiB: clear of the ~4085-cluster FAT12/16
     * boundary, so the formatter lands FAT16 rather than FAT12. */

    enum
    {
        SEMIHOSTING_DISK_SECTOR_SIZE = 512,
        SEMIHOSTING_DISK_SECTOR_COUNT = 16384,
    };

    enum SemihostingDiskResult
    {
        SEMIHOSTING_DISK_OK = 0,
        SEMIHOSTING_DISK_OUT_OF_RANGE,
        SEMIHOSTING_DISK_IO_ERROR,
    };

    /* Open the host image, creating + sparse-extending a fresh zero-filled 8 MiB
     * file on first use. Idempotent — repeated calls short-circuit on the cached
     * handle. Returns true once a usable handle is held. */
    bool SemihostingDisk_EnsureReady(void);

    /* True once EnsureReady has acquired a handle. */
    bool SemihostingDisk_IsReady(void);

    /* Read / write `count` consecutive 512 B sectors starting at LBA `sector`.
     * Both validate the range against the disk geometry before touching the
     * image. The caller is responsible for checking readiness first. */
    enum SemihostingDiskResult SemihostingDisk_Read(void* buffer, uint32_t sector, uint32_t count);
    enum SemihostingDiskResult SemihostingDisk_Write(const void* buffer, uint32_t sector, uint32_t count);

#ifdef __cplusplus
}
#endif

#endif /* SEMIHOSTINGDISK_H */
