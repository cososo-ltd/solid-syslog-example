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

    /* The log source seam, and the only one Minimal occupies. At Baseline it does
     * nothing — created, reaches its idle point, blocks. From Minimal the application logs from
     * (SolidSyslog produces records here). Its stack high-water mark is one of
     * the two figures we track a delta on. */
    bool LogTask_Create(void);
    TaskHandle_t LogTask_Handle(void);

    /* Block until the task has reached its idle point at least once (so its
     * stack high-water mark reflects real scheduling). */
    bool LogTask_WaitIdle(uint32_t timeoutMs);

    /* Have the log source emit one record, and wait for it to finish. Logging
     * happens on this task rather than on the caller's on purpose: a
     * passthrough buffer sends inline, so the send runs on whichever stack calls
     * SolidSyslog_Log. Only by logging from the seam we measure does the log
     * stack figure mean anything. */
    bool LogTask_EmitOnce(uint32_t timeoutMs);

#ifdef __cplusplus
}
#endif

#endif /* APP_TASKS_LOG_TASK_H */
