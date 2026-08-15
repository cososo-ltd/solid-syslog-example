#ifndef APP_TASKS_LOG_TASK_H
#define APP_TASKS_LOG_TASK_H

#include "FreeRTOS.h"
#include "task.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /* The log source seam. With a buffer in front of the sender any task could
     * call Log for the same cost; this stays one task so the stack figure has a
     * single owner. */
    bool LogTask_Create(void);
    TaskHandle_t LogTask_Handle(void);

    /* Block until the task has been scheduled at least once, so its stack
     * high-water mark reflects something real. */
    bool LogTask_WaitIdle(uint32_t timeoutMs);

    /* Emit one record and wait for it to finish. */
    bool LogTask_EmitOnce(uint32_t timeoutMs);

#ifdef __cplusplus
}
#endif

#endif /* APP_TASKS_LOG_TASK_H */
