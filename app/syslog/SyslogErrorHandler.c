/* See SyslogErrorHandler.h. Prints to the same console as the rest of the
 * device, so a fault shows up in the run report next to everything else. */

#include "SyslogErrorHandler.h"

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogPrival.h"

#include <stdio.h>

/* Each name function answers NULL for a value it has no name for: a category
 * in the integrator range, a severity off the ladder, a detail outside its
 * source's vocabulary. */
static const char* OrDash(const char* name)
{
    return (name != NULL) ? name : "-";
}

/* Runs on the task that met the fault, often part way through a SolidSyslog
 * call, so it records and returns: nothing here calls back into the library. */
static void OnSyslogError(void* context, const struct SolidSyslogErrorEvent* event)
{
    (void) context;

    if (event == NULL)
    {
        return;
    }

    /* Sources are matched by pointer identity - Name is only ever a label, which
     * is all it is used for here. */
    (void) printf(
        "[syslog] %s %s %s %s (%ld)",
        OrDash(SolidSyslogSeverity_Name((int32_t) event->Severity)),
        OrDash((event->Source != NULL) ? event->Source->Name : NULL),
        OrDash(SolidSyslogErrorCategory_Name(event->Category)),
        OrDash(SolidSyslogErrorEvent_DetailName(event)),
        (long) event->Detail
    );
    /* The platform's own code - an FRESULT, an errno - has no name, and 0 can be
     * a real one, so it is read only when the event says it is set. */
    if (event->HasNativeCode)
    {
        (void) printf(" native %ld", (long) event->NativeCode);
    }
    (void) printf("\n");
}

void SyslogErrorHandler_Install(void)
{
    SolidSyslog_SetErrorHandler(OnSyslogError, NULL);
}
