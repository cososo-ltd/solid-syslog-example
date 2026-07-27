/* Baseline device — FreeRTOS + lwIP + mbedTLS + FatFs on QEMU mps2-an385,
 * with ZERO SolidSyslog.
 *
 * This is the "simulated existing application": a device that already networks,
 * already uses storage, and has TLS on board — the frozen platform SolidSyslog's
 * footprint deltas are measured against. main() is deliberately thin: it starts
 * the console, creates the two idle application seams (a log source and a service
 * worker), and hands off to a harness task that brings up the simulated existing
 * application, measures the cost above the frozen baseline, and exits.
 *
 * From Minimal, SolidSyslog occupies the two seams; the same measurement then reports
 * exactly what it costs — nothing else in the image moves. */

#include "AppConfig.h"

#include "CmsdkUart.h"
#include "DeviceClock.h"
#include "LogTask.h"
#include "Measure.h"
#include "SemihostingExit.h"
#include "ServiceTask.h"
#include "SimulatedExistingApp.h"
#include "Syslog.h"
#include "SyslogErrorHandler.h"

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
    (void) printf("[device]   sim app (lwIP up, FatFs mounted, mbedTLS linked): %s\n", simReady ? "ready" : "FAILED");

    /* Make sure both idle seams have been scheduled so their stack high-water
     * marks are meaningful (idle, but real). */
    bool logIdle = LogTask_WaitIdle(2000U);
    bool serviceIdle = ServiceTask_WaitIdle(2000U);

    /* Now the network is up, have the log source emit one record. It goes out
     * inline on that task's stack, which is why the log-stack figure below is
     * measured after this and not before. */
    bool logged = LogTask_EmitOnce(5000U);
    (void) printf("[device]   first record logged: %s\n", logged ? "yes" : "FAILED");

    (void) Measure_Report();

    bool ready = simReady && logIdle && serviceIdle && logged;
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

    /* Before anything SolidSyslog, including the first _Create. Every _Create
     * degrades to a Null object rather than failing, so without a handler
     * installed first, a mis-wired logger is indistinguishable from a quiet
     * one. Nothing calls into SolidSyslog yet, so this only costs image space
     * for now — it earns its place from Minimal. */
    SyslogErrorHandler_Install();

    /* lwIP tcpip thread + core-lock mutex + mbox. Pre-scheduler safe. */
    tcpip_init(NULL, NULL);

    /* After tcpip_init, not before: the marshal Syslog_Start installs takes the
     * lwIP core lock, and tcpip_init is what creates it. Nothing is sent here —
     * the sender resolves and opens lazily on its first record. */
    Syslog_Start();

    if (!LogTask_Create() || !ServiceTask_Create())
    {
        (void) printf("[device] FATAL: application task create failed\n");
        SemihostingExit(1);
    }
    if (xTaskCreate(HarnessTask, "harness", HARNESS_TASK_STACK_WORDS, NULL, HARNESS_TASK_PRIORITY, NULL) != pdPASS)
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
