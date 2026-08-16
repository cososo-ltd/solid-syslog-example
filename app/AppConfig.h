/* Compile-time configuration for the device. */
#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include "FreeRTOS.h"
#include "task.h"

#include <stdint.h>

/* CMSDK UART0 on the mps2-an385, surfaced by QEMU over -serial stdio. */
#define DEVICE_UART0_BASE ((uintptr_t) 0x40004000U)

/* Twice the measured high-water mark, rounded up to a whole configMINIMAL_STACK_SIZE,
 * or the FreeRTOS floor where that is below it. The reported figure is high-water
 * usage, which does not depend on the allocation. */
#define LOG_TASK_STACK_WORDS (configMINIMAL_STACK_SIZE * 3U)
#define SERVICE_TASK_STACK_WORDS (configMINIMAL_STACK_SIZE * 15U)
#define LOG_TASK_PRIORITY (tskIDLE_PRIORITY + 1U)
#define SERVICE_TASK_PRIORITY (tskIDLE_PRIORITY + 1U)

/* Bring-up runs here, which makes the broker handshake the deepest stack in the
 * image — so the harness gets the same two-fold rule as the seams. */
#define HARNESS_TASK_STACK_WORDS (configMINIMAL_STACK_SIZE * 12U)
#define HARNESS_TASK_PRIORITY (tskIDLE_PRIORITY + 1U)

/* Read via semihosting, so the path is relative to QEMU's working directory. */
#define BASELINE_FILE_PATH "measurements/Baseline.csv"

/* Peak times 1.5, rounded up to the next KiB. The margin is fragmentation
 * headroom, not spare capacity: buffer_alloc hands out contiguous space, so a
 * buffer only a little over the peak fails on fragmentation rather than on
 * capacity. Applied again wherever more is asked of mbedTLS. */
#define SIMULATED_APP_MBEDTLS_HEAP_BYTES (55 * 1024)

#endif /* APP_CONFIG_H */
