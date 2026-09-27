/* See SyslogPipelineSd.h. */

#include "SyslogPipelineSd.h"

#include "SyslogEnterprise.h"

#include "SolidSyslogSdElement.h"
#include "SolidSyslogSdValue.h"
#include "SolidSyslogStructuredDataDefinition.h"

static const char* s_transport = "tls";

/* A non-zero enterprise number is what makes the SD-ID private: _Begin emits
 * "name@number" for one, a bare IANA "name" for 0. */
static void SyslogPipelineSd_Format(struct SolidSyslogStructuredData* base, struct SolidSyslogSdElement* element)
{
    (void) base;

    SolidSyslogSdElement_Begin(element, "logPipeline", SYSLOG_ENTERPRISE_NUMBER);
    SolidSyslogSdValue_String(SolidSyslogSdElement_Param(element, "transport"), s_transport);
    SolidSyslogSdValue_String(SolidSyslogSdElement_Param(element, "atRest"), "hmac-sha256");
    SolidSyslogSdElement_End(element);
}

/* No _Create and no pool slot: the library never allocates an SD source, so this
 * one is a vtable the application owns. */
static struct SolidSyslogStructuredData s_pipelineSd = {SyslogPipelineSd_Format};

struct SolidSyslogStructuredData* SyslogPipelineSd_Init(bool mutualTls)
{
    s_transport = mutualTls ? "mtls" : "tls";
    return &s_pipelineSd;
}
