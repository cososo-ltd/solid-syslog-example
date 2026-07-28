#ifndef APP_MEASURE_MEASURE_H
#define APP_MEASURE_MEASURE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /* The figures the device self-measures, as an ordered array so the report,
     * the baseline file, and the CI self-check all agree on names and order.
     * Signed, so "used above baseline" can be negative (jitter/tolerance). */
    enum
    {
        MEASURE_FLASH_TEXT = 0, /* .text + .rodata + init arrays (bytes) */
        MEASURE_FLASH_DATA, /* .data (bytes) */
        MEASURE_STATIC_BSS, /* .bss (bytes; includes the reserved FreeRTOS heap array) */
        MEASURE_HEAP_USED, /* configTOTAL_HEAP_SIZE - free heap (bytes) */
        MEASURE_HEAP_PEAK, /* configTOTAL_HEAP_SIZE - lowest free heap ever (bytes) */
        MEASURE_MBEDTLS_PEAK, /* high-water mark of the static mbedTLS buffer (bytes) */
        MEASURE_STACK_LOG, /* log task peak stack used (bytes) */
        MEASURE_STACK_SERVICE, /* service task peak stack used (bytes) */
        MEASURE_KEY_COUNT
    };

    typedef struct
    {
        int32_t value[MEASURE_KEY_COUNT];
    } MeasureValues;

    /* Ordered key names, indexed by the enum above. */
    extern const char* const MEASURE_KEYS[MEASURE_KEY_COUNT];

    /* Fill `out` with the device's current absolute figures. */
    void Measure_Current(MeasureValues* out);

    /* Self-measure, load the baseline (measurements/Baseline.csv; zeros if absent),
     * and print the machine-readable [report] block:
     *
     *   [report] key,current,baseline,used_above_baseline
     *
     * Returns true if a baseline file was found (used above == 0 then means the
     * frozen baseline still matches this image — the self-check). */
    bool Measure_Report(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_MEASURE_MEASURE_H */
