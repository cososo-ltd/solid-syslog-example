/* Entry point, and the harness task that drives one measured run. */

#include "AppConfig.h"

#include "CmsdkUart.h"
#include "DeviceClock.h"
#include "LogTask.h"
#include "Measure.h"
#include "SemihostingExit.h"
#include "ServiceTask.h"
#include "SimulatedExistingApp.h"

#include "lwip/tcpip.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stdint.h>
#include <stdio.h>

/* ---- CMSDK UART0 console (printf -> -serial stdio via Syscalls.c) --------- */

static uint32_t Mmio_Read32(uintptr_t address)
{
    return *(volatile uint32_t*) address;
}

static void Mmio_Write32(uintptr_t address, uint32_t value)
{
    *(volatile uint32_t*) address = value;
}

static void Uart_Sleep(int milliseconds)
{
    /* The QEMU CMSDK UART never reports TX-full, so the polled writer never
     * yields; a no-op keeps CmsdkUart usable for printf before the scheduler. */
    (void) milliseconds;
}

static const CmsdkUartMemoryAccess UART_ACCESS = {
    .read32 = Mmio_Read32,
    .write32 = Mmio_Write32,
    .sleep = Uart_Sleep,
};

/* ---- harness task -------------------------------------------------------- */

static void HarnessTask(void* parameters)
{
    (void) parameters;

    /* Acquire the time before anything that stamps with it. */
    DeviceClock_Start();

    (void) printf("[device] starting simulated existing application...\n");
    bool simReady = SimulatedExistingApp_Start();
    (void) printf(
        "[device]   sim app (lwIP up, FatFs mounted, broker session held over mTLS): %s\n",
        simReady ? "ready" : "FAILED"
    );

    /* Make sure both idle seams have been scheduled so their stack high-water
     * marks are meaningful (idle, but real). */
    bool logIdle = LogTask_WaitIdle(2000U);
    bool serviceIdle = ServiceTask_WaitIdle(2000U);

    (void) Measure_Report();

    bool ready = simReady && logIdle && serviceIdle;
    (void) printf("[device] %s\n", ready ? "ready" : "FAILED");
    SemihostingExit(ready ? 0 : 1);
}

/* ---- FreeRTOS hooks (required by FreeRTOSConfig.h) ------------------------ */

void vApplicationMallocFailedHook(void)
{
    (void) printf("[device] FATAL: malloc failed\n");
    SemihostingExit(1);
}

void vApplicationStackOverflowHook(TaskHandle_t task, char* taskName)
{
    (void) task;
    (void) printf("[device] FATAL: stack overflow in task %s\n", (taskName != NULL) ? taskName : "?");
    SemihostingExit(1);
}

/* ---- entry --------------------------------------------------------------- */

int main(void)
{
    CmsdkUart_Init(&UART_ACCESS, DEVICE_UART0_BASE);
    (void) printf("[device] solid-syslog-example (FreeRTOS + lwIP + mbedTLS + FatFs)\n");

    /* lwIP tcpip thread + core-lock mutex + mbox. Pre-scheduler safe. */
    tcpip_init(NULL, NULL);

    /* Before the scheduler: anything taking an mbedTLS handle needs one built
     * after the allocator was set. */
    if (!SimulatedExistingApp_StartCrypto())
    {
        (void) printf("[device] FATAL: device crypto unavailable\n");
        SemihostingExit(1);
    }

    if (!LogTask_Create() || !ServiceTask_Create())
    {
        (void) printf("[device] FATAL: application task create failed\n");
        SemihostingExit(1);
    }
    static StaticTask_t harnessTaskBuffer;
    static StackType_t harnessStack[HARNESS_TASK_STACK_WORDS];

    if (xTaskCreateStatic(
            HarnessTask,
            "harness",
            HARNESS_TASK_STACK_WORDS,
            NULL,
            HARNESS_TASK_PRIORITY,
            harnessStack,
            &harnessTaskBuffer
        ) == NULL)
    {
        (void) printf("[device] FATAL: harness task create failed\n");
        SemihostingExit(1);
    }

    vTaskStartScheduler();

    for (;;)
    {
    }
    return 0;
}
