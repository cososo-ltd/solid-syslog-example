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

    /* The service seam. At Baseline it does nothing — created, reaches idle, blocks.
     * From Minimal this is where the SolidSyslog Service worker drains the buffer and
     * sends (and, from Secure, runs the TLS handshake — hence the larger stack). Its
     * stack high-water mark is the second figure we track a delta on. */
    bool ServiceTask_Create(void);
    TaskHandle_t ServiceTask_Handle(void);

    bool ServiceTask_WaitIdle(uint32_t timeoutMs);

#ifdef __cplusplus
}
#endif

#endif /* APP_TASKS_SERVICE_TASK_H */
