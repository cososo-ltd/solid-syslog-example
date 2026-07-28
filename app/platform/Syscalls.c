/* Newlib system-call retargeting for the QEMU mps2-an385 FreeRTOS image.
 *
 * Replaces the rdimon (semihosting) syscalls. printf and friends route
 * through _write -> CmsdkUart_Write -> the CMSDK UART0 data register, which
 * QEMU surfaces over `-serial stdio`. _read pulls one byte at a time from
 * CmsdkUart_GetChar (blocking poll on STATE.RXFULL); CR is translated to LF so
 * a terminal sending carriage-return on Enter still terminates fgets, and each
 * byte is echoed back over TX. */

#include "CmsdkUart.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/stat.h>

/* Small heap reserved for newlib re-entrancy buffers (printf's _impure_ptr).
 * Separate from the FreeRTOS heap, and from mbedTLS's own buffer — nothing this
 * device measures allocates here. */
#define SYSCALL_HEAP_SIZE 4096U

static char syscallHeap[SYSCALL_HEAP_SIZE];
static char* syscallHeapBreak = syscallHeap;

static inline bool IsWithinSyscallHeap(const char* candidateBreak);

void* _sbrk(int increment)
{
    char* previousBreak = syscallHeapBreak;
    char* nextBreak = syscallHeapBreak + increment;
    void* result = (void*) -1;
    if (IsWithinSyscallHeap(nextBreak))
    {
        syscallHeapBreak = nextBreak;
        result = previousBreak;
    }
    else
    {
        errno = ENOMEM;
    }
    return result;
}

static inline bool IsWithinSyscallHeap(const char* candidateBreak)
{
    return candidateBreak >= syscallHeap && (size_t) (candidateBreak - syscallHeap) <= sizeof(syscallHeap);
}

int _write(int file, char* buffer, int length)
{
    (void) file;
    if (length > 0)
    {
        CmsdkUart_Write(buffer, (size_t) length);
    }
    return length;
}

int _read(int file, char* buffer, int length)
{
    (void) file;
    int bytesRead = 0;
    if (length > 0)
    {
        char byte = CmsdkUart_GetChar();
        if (byte == '\r')
        {
            byte = '\n';
        }
        if (byte == '\n')
        {
            CmsdkUart_PutChar('\r');
            CmsdkUart_PutChar('\n');
        }
        else
        {
            CmsdkUart_PutChar(byte);
        }
        buffer[0] = byte;
        bytesRead = 1;
    }
    return bytesRead;
}

int _close(int file)
{
    (void) file;
    return -1;
}

int _lseek(int file, int offset, int whence)
{
    (void) file;
    (void) offset;
    (void) whence;
    return 0;
}

int _fstat(int file, struct stat* status)
{
    (void) file;
    status->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int file)
{
    (void) file;
    return 1;
}

int _kill(int pid, int signal)
{
    (void) pid;
    (void) signal;
    errno = EINVAL;
    return -1;
}

int _getpid(void)
{
    return 1;
}

void _exit(int status)
{
    (void) status;
    for (;;)
    {
    }
}
