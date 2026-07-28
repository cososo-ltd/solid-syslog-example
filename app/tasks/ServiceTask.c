/* The idle service seam — see ServiceTask.h. */

#include "ServiceTask.h"

#include "AppConfig.h"
#include "Syslog.h"

#include "SolidSyslog.h"

#include "semphr.h"

/* Poll interval. A missed wake costs latency, never a record: every pass
 * re-derives its state from the live buffer and store. */
#define SERVICE_POLL_MS 20U

static TaskHandle_t s_handle = NULL;
static SemaphoreHandle_t s_reachedIdle = NULL;

static void ServiceTask_Entry(void* parameters)
{
    (void) parameters;

    (void) xSemaphoreGive(s_reachedIdle);

    for (;;)
    {
        (void) SolidSyslog_Service(Syslog_Handle());
        vTaskDelay(pdMS_TO_TICKS(SERVICE_POLL_MS));
    }
}

bool ServiceTask_Create(void)
{
    s_reachedIdle = xSemaphoreCreateBinary();
    if (s_reachedIdle == NULL)
    {
        return false;
    }
    return xTaskCreate(
               ServiceTask_Entry,
               "service",
               SERVICE_TASK_STACK_WORDS,
               NULL,
               SERVICE_TASK_PRIORITY,
               &s_handle
           ) == pdPASS;
}

TaskHandle_t ServiceTask_Handle(void)
{
    return s_handle;
}

bool ServiceTask_WaitIdle(uint32_t timeoutMs)
{
    return xSemaphoreTake(s_reachedIdle, pdMS_TO_TICKS(timeoutMs)) == pdTRUE;
}
