#ifndef APP_PLATFORM_SEMIHOSTING_IO_H
#define APP_PLATFORM_SEMIHOSTING_IO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /* Read a host file into `buffer` (NUL-terminated) over the ARM semihosting
     * trap (BKPT 0xAB) - the same mechanism the FatFs disk uses, available
     * because QEMU is launched with -semihosting-config enable=on. The path is
     * resolved relative to QEMU's working directory. Returns false if the file
     * does not exist or cannot be read (which the baseline loader treats as "no
     * baseline yet"). `*bytesRead` excludes the NUL terminator. */
    bool SemihostingIo_ReadFile(const char* path, char* buffer, size_t bufferSize, size_t* bytesRead);

    /* Create or truncate a host file and write `length` bytes to it. */
    bool SemihostingIo_WriteFile(const char* path, const char* data, size_t length);

    /* The host's wall clock in seconds. The device's own ticks run on QEMU's
     * virtual clock, so anything waiting on the host bounds itself by this. */
    uint32_t SemihostingIo_HostSeconds(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_PLATFORM_SEMIHOSTING_IO_H */
