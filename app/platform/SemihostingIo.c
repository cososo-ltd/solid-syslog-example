/* Host file reads over ARM semihosting — see SemihostingIo.h. Kept separate from
 * storage/SemihostingDisk.c (which is vendored verbatim) so that file stays a
 * clean copy; the BKPT 0xAB trap is duplicated here in a handful of lines. */

#include "SemihostingIo.h"

#include <string.h>

enum
{
    SEMIHOSTING_SYS_OPEN = 0x01,
    SEMIHOSTING_SYS_CLOSE = 0x02,
    SEMIHOSTING_SYS_READ = 0x06,
    SEMIHOSTING_SYS_FLEN = 0x0C,
};

/* ARM semihosting open mode 0 == "r" (read-only text). */
enum
{
    SEMIHOSTING_MODE_READ = 0,
};

static int Semihosting(int operation, const void* args)
{
    register int result __asm("r0") = operation;
    register const void* request __asm("r1") = args;
    __asm volatile("bkpt 0xAB" : "+r"(result) : "r"(request) : "memory");
    return result;
}

static int SemihostingOpen(const char* path, int mode)
{
    const struct
    {
        const char* name;
        int mode;
        int length;
    } args = {path, mode, (int) strlen(path)};

    return Semihosting(SEMIHOSTING_SYS_OPEN, &args);
}

static int SemihostingFlen(int handle)
{
    const struct
    {
        int handle;
    } args = {handle};

    return Semihosting(SEMIHOSTING_SYS_FLEN, &args);
}

static int SemihostingClose(int handle)
{
    const struct
    {
        int handle;
    } args = {handle};

    return Semihosting(SEMIHOSTING_SYS_CLOSE, &args);
}

static int SemihostingRead(int handle, void* buffer, int count)
{
    /* SYS_READ returns the number of bytes NOT read (0 == full read). */
    const struct
    {
        int handle;
        void* buffer;
        int count;
    } args = {handle, buffer, count};

    return Semihosting(SEMIHOSTING_SYS_READ, &args);
}

bool SemihostingIo_ReadFile(const char* path, char* buffer, size_t bufferSize, size_t* bytesRead)
{
    *bytesRead = 0;
    if (bufferSize == 0)
    {
        return false;
    }

    int handle = SemihostingOpen(path, SEMIHOSTING_MODE_READ);
    if (handle < 0)
    {
        return false;
    }

    int length = SemihostingFlen(handle);
    if (length < 0)
    {
        (void) SemihostingClose(handle);
        return false;
    }

    size_t toRead = ((size_t) length < (bufferSize - 1)) ? (size_t) length : (bufferSize - 1);
    int notRead = SemihostingRead(handle, buffer, (int) toRead);
    (void) SemihostingClose(handle);
    if (notRead < 0)
    {
        return false;
    }

    size_t got = toRead - (size_t) notRead;
    buffer[got] = '\0';
    *bytesRead = got;
    return true;
}
