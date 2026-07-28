/* See SyslogErrorHandler.h. Prints to the same console as the rest of the
 * device, so a fault shows up in the run report next to everything else. */

#include "SyslogErrorHandler.h"

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogPrival.h"

#include <stdint.h>
#include <stdio.h>

static const char* SeverityName(enum SolidSyslogSeverity severity)
{
    switch (severity)
    {
        case SOLIDSYSLOG_SEVERITY_EMERGENCY:
            return "EMERGENCY";
        case SOLIDSYSLOG_SEVERITY_ALERT:
            return "ALERT";
        case SOLIDSYSLOG_SEVERITY_CRITICAL:
            return "CRITICAL";
        case SOLIDSYSLOG_SEVERITY_ERROR:
            return "ERROR";
        case SOLIDSYSLOG_SEVERITY_WARNING:
            return "WARNING";
        case SOLIDSYSLOG_SEVERITY_NOTICE:
            return "NOTICE";
        case SOLIDSYSLOG_SEVERITY_INFORMATIONAL:
            return "INFO";
        case SOLIDSYSLOG_SEVERITY_DEBUG:
            return "DEBUG";
    }
    return "?";
}

/* The four a misconfigured integration raises. Role-specific categories sit in
 * ranges above 0x0100 and print numerically. */
static const char* CategoryName(uint16_t category)
{
    switch (category)
    {
        case SOLIDSYSLOG_CAT_BAD_CONFIG:
            return "bad-config";
        case SOLIDSYSLOG_CAT_BAD_ARGUMENT:
            return "bad-argument";
        case SOLIDSYSLOG_CAT_POOL_EXHAUSTED:
            return "pool-exhausted";
        case SOLIDSYSLOG_CAT_UNKNOWN_DESTROY:
            return "unknown-destroy";
        default:
            return NULL;
    }
}

static void OnSyslogError(void* context, const struct SolidSyslogErrorEvent* event)
{
    (void) context;

    if (event == NULL)
    {
        return;
    }

    /* Sources are matched by pointer identity — Name is only ever a label, which
     * is all it is used for here. */
    const char* source = ((event->Source != NULL) && (event->Source->Name != NULL)) ? event->Source->Name : "?";
    const char* category = CategoryName(event->Category);

    if (category != NULL)
    {
        (void) printf("[syslog] %s %s %s (detail %ld)\n", SeverityName(event->Severity), source, category, (long) event->Detail);
    }
    else
    {
        (void) printf(
            "[syslog] %s %s category 0x%04X (detail %ld)\n",
            SeverityName(event->Severity),
            source,
            (unsigned int) event->Category,
            (long) event->Detail
        );
    }
}

void SyslogErrorHandler_Install(void)
{
    SolidSyslog_SetErrorHandler(OnSyslogError, NULL);
}
