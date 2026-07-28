#ifndef APP_TASKS_SERVICE_TASK_H
#define APP_TASKS_SERVICE_TASK_H

#include "FreeRTOS.h"
#include "task.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /* The service seam: where a drain-and-send worker would run. Idle here, so
     * its stack is at the floor; whatever occupies it grows that. Its stack
     * high-water mark is the second figure the run tracks a delta on. */
    bool ServiceTask_Create(void);
    TaskHandle_t ServiceTask_Handle(void);

    bool ServiceTask_WaitIdle(uint32_t timeoutMs);

#ifdef __cplusplus
}
#endif

#endif /* APP_TASKS_SERVICE_TASK_H */
