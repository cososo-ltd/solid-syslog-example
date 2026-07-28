/* ChaN-FatFs disk_* glue over the semihosting media (SemihostingDisk).
 *
 * Maps FatFs's disk_* contract onto a host-backed flat image; the semihosting
 * trap, the 8 MiB FAT16 geometry and the open-or-create-sparse logic live in
 * SemihostingDisk. Single logical drive (pdrv 0); no exFAT, no LFN.
 *
 * mps2-an385 under QEMU has no storage peripheral, so the host stands in — the
 * same standing-in the wall clock and the cert store do. Replacing this file is
 * what a real integration changes. */

/* ff.h before diskio.h: diskio.h declares disk_* in terms of BYTE / UINT /
 * LBA_t / DWORD / WORD which are typedef'd in ff.h's integer headers. */
#include "ff.h"
#include "diskio.h"

#include "DeviceClock.h"
#include "SemihostingDisk.h"

#include <stdbool.h>

static bool Diskio_IsDriveReady(BYTE pdrv);

DSTATUS disk_initialize(BYTE pdrv)
{
    if (pdrv != 0)
    {
        return STA_NOINIT;
    }
    return SemihostingDisk_EnsureReady() ? 0 : STA_NOINIT;
}

DSTATUS disk_status(BYTE pdrv)
{
    if (pdrv != 0)
    {
        return STA_NOINIT;
    }
    return SemihostingDisk_IsReady() ? 0 : STA_NOINIT;
}

DRESULT disk_read(BYTE pdrv, BYTE* buff, LBA_t sector, UINT count)
{
    DRESULT result;
    if (!Diskio_IsDriveReady(pdrv))
    {
        result = RES_NOTRDY;
    }
    else
    {
        switch (SemihostingDisk_Read(buff, (uint32_t) sector, (uint32_t) count))
        {
            case SEMIHOSTING_DISK_OK:
                result = RES_OK;
                break;
            case SEMIHOSTING_DISK_OUT_OF_RANGE:
                result = RES_PARERR;
                break;
            default:
                result = RES_ERROR;
                break;
        }
    }
    return result;
}

/* Single logical drive 0 must be present and the backing image opened. */
static bool Diskio_IsDriveReady(BYTE pdrv)
{
    return (pdrv == 0) && SemihostingDisk_IsReady();
}

DRESULT disk_write(BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count)
{
    DRESULT result;
    if (!Diskio_IsDriveReady(pdrv))
    {
        result = RES_NOTRDY;
    }
    else
    {
        switch (SemihostingDisk_Write(buff, (uint32_t) sector, (uint32_t) count))
        {
            case SEMIHOSTING_DISK_OK:
                result = RES_OK;
                break;
            case SEMIHOSTING_DISK_OUT_OF_RANGE:
                result = RES_PARERR;
                break;
            default:
                result = RES_ERROR;
                break;
        }
    }
    return result;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* buff)
{
    if (pdrv != 0)
    {
        return RES_PARERR;
    }
    switch (cmd)
    {
        case CTRL_SYNC:
            /* QEMU semihosting writes are synchronous against the host
             * file — no kernel-level dirty pages we need to flush. */
            return RES_OK;
        case GET_SECTOR_COUNT:
            *(LBA_t*) buff = (LBA_t) SEMIHOSTING_DISK_SECTOR_COUNT;
            return RES_OK;
        case GET_SECTOR_SIZE:
            *(WORD*) buff = (WORD) SEMIHOSTING_DISK_SECTOR_SIZE;
            return RES_OK;
        case GET_BLOCK_SIZE:
            /* Erase-block size in sectors. The semihosting flat file
             * has no native erase granularity; 1 is the safe minimum. */
            *(DWORD*) buff = 1U;
            return RES_OK;
        default:
            return RES_PARERR;
    }
}

/* FatFs file timestamps, from the device wall clock. Packed as FatFs wants it:
 * year from 1980 in bits 25..31, then month, day, hour, minute, and seconds
 * halved in bits 0..4. A clock that has not been acquired yields 0, which FatFs
 * reads as "no timestamp". */
DWORD get_fattime(void)
{
    struct tm utc;
    uint32_t microseconds = 0U;

    if (!DeviceClock_Now(&utc, &microseconds) || (utc.tm_year < 80))
    {
        return 0U;
    }

    return ((DWORD) (utc.tm_year - 80) << 25) | ((DWORD) (utc.tm_mon + 1) << 21) | ((DWORD) utc.tm_mday << 16)
           | ((DWORD) utc.tm_hour << 11) | ((DWORD) utc.tm_min << 5) | ((DWORD) (utc.tm_sec / 2));
}
