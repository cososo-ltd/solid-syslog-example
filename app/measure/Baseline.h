#ifndef APP_MEASURE_BASELINE_H
#define APP_MEASURE_BASELINE_H

#include "Measure.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /* Load BASELINE_FILE_PATH — `key,value` lines, # comments ignored — via
     * semihosting. False if the file is absent, which the report renders as "no
     * baseline yet".
     *
     * `present[i]` says whether the file carried MEASURE_KEYS[i]: a key the
     * frozen baseline predates must not be reported as a delta against zero. */
    bool Baseline_Load(MeasureValues* out, bool* present);

#ifdef __cplusplus
}
#endif

#endif /* APP_MEASURE_BASELINE_H */
