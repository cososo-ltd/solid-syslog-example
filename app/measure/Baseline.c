/* Frozen-baseline loader — see Baseline.h. */

#include "Baseline.h"

#include "AppConfig.h"
#include "Measure.h"
#include "SemihostingIo.h"

#include <stdlib.h>
#include <string.h>

/* Big enough for the handful of `key,value` lines the baseline holds. */
#define BASELINE_BUFFER_SIZE 1024

static void Baseline_Apply(MeasureValues* out, bool* present, const char* key, const char* value)
{
    for (int i = 0; i < MEASURE_KEY_COUNT; i++)
    {
        if (strcmp(key, MEASURE_KEYS[i]) == 0)
        {
            out->value[i] = (int32_t) strtol(value, NULL, 10);
            present[i] = true;
            return;
        }
    }
}

bool Baseline_Load(MeasureValues* out, bool* present)
{
    for (int i = 0; i < MEASURE_KEY_COUNT; i++)
    {
        out->value[i] = 0;
        present[i] = false;
    }

    static char buffer[BASELINE_BUFFER_SIZE];
    size_t length = 0;
    if (!SemihostingIo_ReadFile(BASELINE_FILE_PATH, buffer, sizeof(buffer), &length))
    {
        return false;
    }

    char* line = buffer;
    while ((line != NULL) && (*line != '\0'))
    {
        char* newline = strchr(line, '\n');
        if (newline != NULL)
        {
            *newline = '\0';
        }
        char* carriage = strchr(line, '\r');
        if (carriage != NULL)
        {
            *carriage = '\0';
        }

        if ((line[0] != '\0') && (line[0] != '#'))
        {
            char* comma = strchr(line, ',');
            if (comma != NULL)
            {
                *comma = '\0';
                Baseline_Apply(out, present, line, comma + 1);
            }
        }

        line = (newline != NULL) ? (newline + 1) : NULL;
    }
    return true;
}
