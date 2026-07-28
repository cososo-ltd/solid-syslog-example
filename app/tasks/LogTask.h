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

    /* The log source seam: the one place this device logs from. Its stack
     * high-water mark is one of the two figures we track a delta on, which only
     * means anything if logging happens here and not on the harness that asks
     * for it. */
    bool LogTask_Create(void);
    TaskHandle_t LogTask_Handle(void);

    /* Block until the task has reached its idle point at least once (so its
     * stack high-water mark reflects real scheduling). */
    bool LogTask_WaitIdle(uint32_t timeoutMs);

    /* Have the log source emit one record, and wait for it to finish. */
    bool LogTask_EmitOnce(uint32_t timeoutMs);

#ifdef __cplusplus
}
#endif

#endif /* APP_TASKS_LOG_TASK_H */
