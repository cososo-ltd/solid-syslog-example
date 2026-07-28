/* Clean QEMU termination over ARM semihosting.
 *
 * Uses the same BKPT 0xAB trap that the FatFs semihosting disk (storage/
 * SemihostingDisk.c) uses for file I/O — the QEMU launch enables semihosting,
 * so SYS_EXIT is available too. On SYS_EXIT the guest passes an ADP_Stopped_*
 * reason in r1: ApplicationExit makes QEMU exit 0, any other reason exits
 * non-zero, which is all the smoke test needs to distinguish pass from fail. */

#include "SemihostingExit.h"

#include <stdint.h>

enum
{
    SEMIHOSTING_SYS_EXIT = 0x18,
    ADP_STOPPED_APPLICATION_EXIT = 0x20026,
    ADP_STOPPED_RUNTIME_ERROR = 0x20023,
};

void SemihostingExit(int code)
{
    uint32_t reason = (code == 0) ? (uint32_t) ADP_STOPPED_APPLICATION_EXIT : (uint32_t) ADP_STOPPED_RUNTIME_ERROR;

    register uint32_t operation __asm("r0") = SEMIHOSTING_SYS_EXIT;
    register uint32_t argument __asm("r1") = reason;
    __asm volatile("bkpt 0xAB" : "+r"(operation) : "r"(argument) : "memory");

    for (;;)
    {
    }
}
