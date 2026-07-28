/* See Measure.h. Flash and RAM come from linker-defined symbols, the rest from
 * FreeRTOS, lwIP and mbedTLS asking themselves. */

#include "Measure.h"

#include "AppConfig.h"
#include "Baseline.h"
#include "LogTask.h"
#include "ServiceTask.h"
#include "SimulatedExistingApp.h"

#include "lwip/memp.h"
#include "lwip/stats.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stdint.h>
#include <stdio.h>

/* Linker-defined section boundaries (see app/platform/mps2-an385.ld). Declared
 * as arrays so the name is the address; the stored value is meaningless. */
extern char _sidata[];
extern char _sdata[];
extern char _edata[];
extern char _sbss[];
extern char _ebss[];

const char* const MEASURE_KEYS[MEASURE_KEY_COUNT] = {
    "flash_text",
    "flash_data",
    "static_bss",
    "heap_used",
    "mbedtls_peak",
    "mbedtls_free",
    "lwip_mem_free",
    "lwip_pbufs_free",
    "stack_log",
    "stack_service",
    "stack_harness",
};

/* lwIP sizes its pools at compile time, so the figure worth reporting is the part
 * of each that was never reached. */
static int32_t Measure_LwipHeapFreeBytes(void)
{
    return (int32_t) lwip_stats.mem.avail - (int32_t) lwip_stats.mem.max;
}

static int32_t Measure_LwipPbufsFree(void)
{
    const struct stats_mem* pool = lwip_stats.memp[MEMP_PBUF_POOL];

    return (int32_t) pool->avail - (int32_t) pool->max;
}

static int32_t Measure_StackUsedBytes(TaskHandle_t task, uint32_t allocatedWords)
{
    uint32_t headroomWords = (uint32_t) uxTaskGetStackHighWaterMark(task);
    uint32_t usedWords = (allocatedWords > headroomWords) ? (allocatedWords - headroomWords) : 0U;
    return (int32_t) (usedWords * (uint32_t) sizeof(StackType_t));
}

void Measure_Current(MeasureValues* out)
{
    /* FLASH origin is 0x0, so the load address of .data (_sidata) is exactly the
     * size of the read-only image (.text + .rodata + init arrays). */
    out->value[MEASURE_FLASH_TEXT] = (int32_t) (uintptr_t) _sidata;
    out->value[MEASURE_FLASH_DATA] = (int32_t) ((uintptr_t) _edata - (uintptr_t) _sdata);
    out->value[MEASURE_STATIC_BSS] = (int32_t) ((uintptr_t) _ebss - (uintptr_t) _sbss);
    out->value[MEASURE_HEAP_USED] = (int32_t) ((uint32_t) configTOTAL_HEAP_SIZE - (uint32_t) xPortGetFreeHeapSize());
    out->value[MEASURE_MBEDTLS_PEAK] = (int32_t) SimulatedExistingApp_MbedTlsPeak();
    out->value[MEASURE_MBEDTLS_FREE] = (int32_t) SimulatedExistingApp_MbedTlsFree();
    out->value[MEASURE_LWIP_MEM_FREE] = Measure_LwipHeapFreeBytes();
    out->value[MEASURE_LWIP_PBUFS_FREE] = Measure_LwipPbufsFree();
    out->value[MEASURE_STACK_LOG] = Measure_StackUsedBytes(LogTask_Handle(), LOG_TASK_STACK_WORDS);
    out->value[MEASURE_STACK_SERVICE] = Measure_StackUsedBytes(ServiceTask_Handle(), SERVICE_TASK_STACK_WORDS);
    /* NULL is the calling task, and Measure_Report runs on the harness. */
    out->value[MEASURE_STACK_HARNESS] = Measure_StackUsedBytes(NULL, HARNESS_TASK_STACK_WORDS);
}

bool Measure_Report(void)
{
    MeasureValues current;
    Measure_Current(&current);

    MeasureValues baseline;
    bool present[MEASURE_KEY_COUNT];
    bool haveBaseline = Baseline_Load(&baseline, present);

    (void) printf("[report] --- SolidSyslog cost above baseline (simulated existing application) ---\n");
    (void) printf("[report] key,current,baseline,used_above_baseline\n");
    for (int i = 0; i < MEASURE_KEY_COUNT; i++)
    {
        int32_t currentValue = current.value[i];
        if (haveBaseline && !present[i])
        {
            /* The frozen baseline predates this key. Reporting current-minus-zero
             * would look like a delta and be read as one. */
            (void) printf("[report] %s,%ld,-,-\n", MEASURE_KEYS[i], (long) currentValue);
            continue;
        }
        int32_t baselineValue = haveBaseline ? baseline.value[i] : 0;
        int32_t usedAbove = currentValue - baselineValue;
        (void) printf(
            "[report] %s,%ld,%ld,%ld\n",
            MEASURE_KEYS[i],
            (long) currentValue,
            (long) baselineValue,
            (long) usedAbove
        );
    }
    (void) printf("[report] --- end ---\n");

    if (!haveBaseline)
    {
        (void) printf(
            "[report] note: no baseline file (%s) — 'used_above' equals current; "
            "commit these absolutes as the baseline.\n",
            BASELINE_FILE_PATH
        );
    }
    return haveBaseline;
}
