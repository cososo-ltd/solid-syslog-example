/* Shared configuration for the Baseline.
 *
 * The baseline is the "simulated existing application": FreeRTOS running, lwIP
 * up, FatFs mounted, and mbedTLS linked but never run. Two application tasks —
 * a log source and a service worker — sit idle, ready for SolidSyslog to occupy
 * from Minimal. We measure only what SolidSyslog adds on top of this frozen baseline. */
#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include "FreeRTOS.h"
#include "task.h"

#include <stdint.h>

/* CMSDK UART0 on the mps2-an385, surfaced by QEMU over -serial stdio. */
#define DEVICE_UART0_BASE ((uintptr_t) 0x40004000U)

/* The two application seams SolidSyslog may occupy:
 *   - log     : the task the application logs FROM (producer)   [from Minimal]
 *   - service : the drain/send worker (SolidSyslog Service)     [from Secure]
 * Minimal occupies only the first — it sends inline on the logging task and adds
 * no thread at all. The service worker starts costing at Secure, when a queue
 * and store-and-forward give it something to drain.
 * Both are idle at Baseline. Their stacks are sized generously for the full Hardened
 * build and FROZEN at Baseline — only *usage* moves across tags, never the allocation,
 * so the stack figures we publish are always a delta on the same allocation.
 * Sizes are in FreeRTOS stack words (StackType_t). */
#define LOG_TASK_STACK_WORDS (configMINIMAL_STACK_SIZE * 4U) /* ~2 KiB */
#define SERVICE_TASK_STACK_WORDS \
    (configMINIMAL_STACK_SIZE * 16U) /* ~8 KiB — the TLS handshake path is the deepest */
#define LOG_TASK_PRIORITY (tskIDLE_PRIORITY + 1U)
#define SERVICE_TASK_PRIORITY (tskIDLE_PRIORITY + 1U)

/* The harness task brings up the simulated existing application, waits for the
 * two seams to reach idle, measures, and exits. It is test scaffolding, not part
 * of the device, and is not itself measured. */
#define HARNESS_TASK_STACK_WORDS (configMINIMAL_STACK_SIZE * 8U)
#define HARNESS_TASK_PRIORITY (tskIDLE_PRIORITY + 1U)

/* Frozen baseline figures (Baseline absolutes), read via semihosting relative to
 * QEMU's working directory. Absent on the first-ever run → baseline reads 0, and
 * the report prints absolutes for us to commit as the baseline. */
#define BASELINE_FILE_PATH "measurements/Baseline.csv"

/* The static buffer mbedTLS sub-allocates from. Sized at the high-water mark the
 * device reports times 1.5, rounded up to the next KiB — buffer_alloc hands out
 * contiguous space, so a buffer only a little over the peak fails on
 * fragmentation rather than on capacity. One rule, applied again at every step
 * that asks more of mbedTLS, so the baseline holds no spare capacity on a later
 * step's behalf and each step is charged what it actually added. */
#define SIMULATED_APP_MBEDTLS_HEAP_BYTES (55 * 1024)

#endif /* APP_CONFIG_H */
