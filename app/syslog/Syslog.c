/* See Syslog.h.
 *
 * At this step the logger is created with nothing wired into it, on purpose.
 * SolidSyslog_Create never fails and never returns NULL: a missing collaborator
 * is substituted with its Null object and reported through the error handler.
 * Creating it empty first is how this example shows what that looks like — the
 * run report carries the faults, and the next step makes them go quiet by
 * wiring a buffer and a sender. */

#include "Syslog.h"

#include "SolidSyslogConfig.h"

#include <stddef.h>

static struct SolidSyslog* s_logger = NULL;

void Syslog_Start(void)
{
    /* Buffer and Sender are the two collaborators that decide where a record
     * goes. Both absent for now. NULL here means "not supplied" and is reported;
     * from the next step, a collaborator we deliberately do without is passed as
     * its Null object instead, which is how the library tells "I meant this"
     * apart from "I forgot". */
    struct SolidSyslogConfig config = {
        .Buffer = NULL,
        .Sender = NULL,
    };

    /* No null check: Create returns a shared null instance rather than NULL on
     * any failure, so callers never have to test it. What it does not do is stay
     * silent — that is the error handler's job. */
    s_logger = SolidSyslog_Create(&config);
}

struct SolidSyslog* Syslog_Handle(void)
{
    return s_logger;
}
