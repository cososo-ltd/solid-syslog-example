#ifndef APP_PLATFORM_SEMIHOSTING_EXIT_H
#define APP_PLATFORM_SEMIHOSTING_EXIT_H

#ifdef __cplusplus
extern "C"
{
#endif

    /* Terminate the QEMU guest via the ARM semihosting SYS_EXIT call (BKPT
     * 0xAB). QEMU is launched with -semihosting-config enable=on, so this
     * cleanly stops the emulator: exit code 0 for `code == 0`, non-zero
     * otherwise. Does not return. */
    void SemihostingExit(int code);

#ifdef __cplusplus
}
#endif

#endif /* APP_PLATFORM_SEMIHOSTING_EXIT_H */
