/* The idle log-source seam — see LogTask.h. */

#include "LogTask.h"

#include "AppConfig.h"

#include "semphr.h"

static TaskHandle_t s_handle = NULL;
static SemaphoreHandle_t s_reachedIdle = NULL;

static void LogTask_Entry(void* parameters)
{
    (void) parameters;

    /* At Baseline the log source does nothing. From Minimal SolidSyslog logs from here. */
    (void) xSemaphoreGive(s_reachedIdle);

    for (;;)
    {
        vTaskDelay(portMAX_DELAY);
    }
}

bool LogTask_Create(void)
{
    s_reachedIdle = xSemaphoreCreateBinary();
    if (s_reachedIdle == NULL)
    {
        return false;
    }
    return xTaskCreate(LogTask_Entry, "log", LOG_TASK_STACK_WORDS, NULL, LOG_TASK_PRIORITY, &s_handle) == pdPASS;
}

TaskHandle_t LogTask_Handle(void)
{
    return s_handle;
}

bool LogTask_WaitIdle(uint32_t timeoutMs)
{
    return xSemaphoreTake(s_reachedIdle, pdMS_TO_TICKS(timeoutMs)) == pdTRUE;
}
