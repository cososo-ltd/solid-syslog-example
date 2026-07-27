/* See DeviceClock.h. */

#include "DeviceClock.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stddef.h>

enum
{
    SEMIHOSTING_SYS_TIME = 0x11
};

static uint32_t s_acquiredUnixSeconds = 0U;
static TickType_t s_acquiredTick = 0U;
static bool s_acquired = false;

/* Same BKPT 0xAB trap the semihosting disk and exit paths use; SYS_TIME returns
 * seconds since the Unix epoch in r0. */
static uint32_t HostUnixTime(void)
{
    register uint32_t operation __asm("r0") = SEMIHOSTING_SYS_TIME;
    register uint32_t argument __asm("r1") = 0U;
    __asm volatile("bkpt 0xAB" : "+r"(operation) : "r"(argument) : "memory");
    return operation;
}

void DeviceClock_Start(void)
{
    const uint32_t acquired = HostUnixTime();
    if (acquired == 0U)
    {
        return;
    }

    s_acquiredTick = xTaskGetTickCount();
    s_acquiredUnixSeconds = acquired;
    s_acquired = true;
}

bool DeviceClock_Now(struct tm* utc, uint32_t* microseconds)
{
    if (!s_acquired || (utc == NULL) || (microseconds == NULL))
    {
        return false;
    }

    /* Unsigned subtraction, so a tick wrap still yields the right elapsed count.
     * 32-bit throughout: a Cortex-M3 has no 64-bit divide. */
    const TickType_t elapsed = xTaskGetTickCount() - s_acquiredTick;
    const time_t seconds = (time_t) (s_acquiredUnixSeconds + (uint32_t) (elapsed / configTICK_RATE_HZ));

    if (gmtime_r(&seconds, utc) == NULL)
    {
        return false;
    }

    *microseconds = (uint32_t) (elapsed % configTICK_RATE_HZ) * (1000000UL / configTICK_RATE_HZ);
    return true;
}
