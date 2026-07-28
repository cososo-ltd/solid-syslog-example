#ifndef APP_MEASURE_BASELINE_H
#define APP_MEASURE_BASELINE_H

#include "Measure.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /* Load the frozen baseline (Baseline absolutes) from BASELINE_FILE_PATH via
     * semihosting. The file is `key,value` lines (# comments ignored). Zeros
     * every field first, so a missing file yields an all-zero baseline and
     * returns false — which the report renders as "no baseline yet, commit
     * these absolutes". Returns true if the file was read. */
    /* `present[i]` says whether the file actually carried MEASURE_KEYS[i]. A key a
     * frozen baseline predates must not be reported as a delta against zero. */
    bool Baseline_Load(MeasureValues* out, bool* present);

#ifdef __cplusplus
}
#endif

#endif /* APP_MEASURE_BASELINE_H */
