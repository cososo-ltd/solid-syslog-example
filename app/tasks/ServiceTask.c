/* The idle service seam — see ServiceTask.h. */

#include "ServiceTask.h"

#include "AppConfig.h"

#include "semphr.h"

static TaskHandle_t s_handle = NULL;
static SemaphoreHandle_t s_reachedIdle = NULL;

static void ServiceTask_Entry(void* parameters)
{
    (void) parameters;

    /* At Baseline the service worker does nothing. From Minimal the SolidSyslog Service
     * drains and sends here. */
    (void) xSemaphoreGive(s_reachedIdle);

    for (;;)
    {
        vTaskDelay(portMAX_DELAY);
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
