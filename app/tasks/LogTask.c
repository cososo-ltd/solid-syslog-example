/* The log-source seam — see LogTask.h. */

#include "LogTask.h"

#include "AppConfig.h"

#include "semphr.h"

static TaskHandle_t s_handle = NULL;
static SemaphoreHandle_t s_reachedIdle = NULL;
static SemaphoreHandle_t s_emitRequested = NULL;
static SemaphoreHandle_t s_emitDone = NULL;

static void LogTask_Entry(void* parameters)
{
    (void) parameters;

    (void) xSemaphoreGive(s_reachedIdle);

    for (;;)
    {
        if (xSemaphoreTake(s_emitRequested, portMAX_DELAY) == pdTRUE)
        {
            /* Nothing to say yet — a later step gives this device a logger. */
            (void) xSemaphoreGive(s_emitDone);
        }
    }
}

bool LogTask_Create(void)
{
    s_reachedIdle = xSemaphoreCreateBinary();
    s_emitRequested = xSemaphoreCreateBinary();
    s_emitDone = xSemaphoreCreateBinary();
    if ((s_reachedIdle == NULL) || (s_emitRequested == NULL) || (s_emitDone == NULL))
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

bool LogTask_EmitOnce(uint32_t timeoutMs)
{
    (void) xSemaphoreGive(s_emitRequested);
    return xSemaphoreTake(s_emitDone, pdMS_TO_TICKS(timeoutMs)) == pdTRUE;
}
