/* See Syslog.h. Created with nothing wired into it, on purpose: a missing
 * collaborator is substituted with its Null object and reported, and the run
 * report is where that shows. */

#include "Syslog.h"

#include "SolidSyslogConfig.h"

#include <stddef.h>

static struct SolidSyslog* s_logger = NULL;

void Syslog_Start(void)
{
    /* Buffer and Sender decide where a record goes. NULL is "not supplied" and
     * is reported; a collaborator deliberately done without is passed as its
     * Null object instead, which is how the library tells the two apart. */
    struct SolidSyslogConfig config = {
        .Buffer = NULL,
        .Sender = NULL,
    };

    /* No null check — Create returns a shared null instance rather than NULL. */
    s_logger = SolidSyslog_Create(&config);
}

struct SolidSyslog* Syslog_Handle(void)
{
    return s_logger;
}
