/* See SimulatedCollector.h. The request and its acknowledgement are files the
 * collector container watches, reached over semihosting. */

#include "SimulatedCollector.h"

#include "SemihostingIo.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stddef.h>

#define NEXT_PIN_PATH "build/certs/collector-next.pin"
#define RENEW_REQUEST_PATH "build/collector/renew"
#define RENEWED_PATH "build/collector/renewed"

/* sha-256: plus 32 colon-separated hex pairs is 103 characters. */
static char s_nextPin[128];

const char* SimulatedCollector_NextPin(void)
{
    size_t length = 0;
    if (!SemihostingIo_ReadFile(NEXT_PIN_PATH, s_nextPin, sizeof(s_nextPin), &length))
    {
        return NULL;
    }
    /* The file ends with a newline; the library wants the pin and nothing else. */
    while ((length > 0U) && (s_nextPin[length - 1U] <= ' '))
    {
        length--;
    }
    s_nextPin[length] = '\0';
    return s_nextPin;
}

bool SimulatedCollector_Renew(uint32_t timeoutSeconds)
{
    if (!SemihostingIo_WriteFile(RENEW_REQUEST_PATH, "renew\n", 6U))
    {
        return false;
    }

    /* Bounded by the host's clock: the restart takes real time, and the device's
     * ticks can outrun it. */
    const uint32_t start = SemihostingIo_HostSeconds();
    char ack[8];
    size_t length = 0;
    while ((SemihostingIo_HostSeconds() - start) < timeoutSeconds)
    {
        if (SemihostingIo_ReadFile(RENEWED_PATH, ack, sizeof(ack), &length))
        {
            return true;
        }
        vTaskDelay(pdMS_TO_TICKS(100U));
    }
    return false;
}
