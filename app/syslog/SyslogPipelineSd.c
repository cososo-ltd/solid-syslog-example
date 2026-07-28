/* See SyslogPipelineSd.h. */

#include "SyslogPipelineSd.h"

#include "SyslogEnterprise.h"

#include "SolidSyslogSdElement.h"
#include "SolidSyslogSdValue.h"
#include "SolidSyslogStructuredDataDefinition.h"

/* The weakest honest answer until Init says otherwise. A missing credential
 * downgrades the transport or the store policy without failing anything, so
 * claiming protection the device does not have would defeat the point of
 * reporting it at all. */
static const char* s_transport = "tls";
static const char* s_atRest = "none";

/* Called once per record. A non-zero enterprise number is what makes the SD-ID
 * private — _Begin emits "name@number" for one, a bare IANA "name" for 0. The
 * element writer owns the brackets, the SD-NAME charset and the escaping, so this
 * supplies only names and values and cannot desync the framing. */
static void SyslogPipelineSd_Format(struct SolidSyslogStructuredData* base, struct SolidSyslogSdElement* element)
{
    (void) base;

    SolidSyslogSdElement_Begin(element, "logPipeline", SYSLOG_ENTERPRISE_NUMBER);
    SolidSyslogSdValue_String(SolidSyslogSdElement_Param(element, "transport"), s_transport);
    SolidSyslogSdValue_String(SolidSyslogSdElement_Param(element, "atRest"), s_atRest);
    SolidSyslogSdElement_End(element);
}

/* No _Create and no pool slot: the library never allocates an SD source, so a
 * stateless one is a vtable this application owns. */
static struct SolidSyslogStructuredData s_pipelineSd = {SyslogPipelineSd_Format};

struct SolidSyslogStructuredData* SyslogPipelineSd_Init(const char* transport, const char* atRest)
{
    s_transport = transport;
    s_atRest = atRest;
    return &s_pipelineSd;
}
