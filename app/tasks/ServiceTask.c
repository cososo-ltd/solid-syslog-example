/* The idle service seam — see ServiceTask.h. */

#include "ServiceTask.h"

#include "AppConfig.h"

#include "semphr.h"

static TaskHandle_t s_handle = NULL;
static StaticTask_t s_taskBuffer;
static StackType_t s_stack[SERVICE_TASK_STACK_WORDS];
static SemaphoreHandle_t s_reachedIdle = NULL;
static StaticSemaphore_t s_reachedIdleBuffer;

static void ServiceTask_Entry(void* parameters)
{
    (void) parameters;

    (void) xSemaphoreGive(s_reachedIdle);

    for (;;)
    {
        vTaskDelay(portMAX_DELAY);
    }
}

bool ServiceTask_Create(void)
{
    s_reachedIdle = xSemaphoreCreateBinaryStatic(&s_reachedIdleBuffer);
    if (s_reachedIdle == NULL)
    {
        return false;
    }
    s_handle = xTaskCreateStatic(
        ServiceTask_Entry,
        "service",
        SERVICE_TASK_STACK_WORDS,
        NULL,
        SERVICE_TASK_PRIORITY,
        s_stack,
        &s_taskBuffer
    );
    return s_handle != NULL;
}

TaskHandle_t ServiceTask_Handle(void)
{
    return s_handle;
}

bool ServiceTask_WaitIdle(uint32_t timeoutMs)
{
    return xSemaphoreTake(s_reachedIdle, pdMS_TO_TICKS(timeoutMs)) == pdTRUE;
}
