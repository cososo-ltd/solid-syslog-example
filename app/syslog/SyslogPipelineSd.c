/* See SyslogPipelineSd.h. */

#include "SyslogPipelineSd.h"

#include "SyslogEnterprise.h"

#include "SolidSyslogSdElement.h"
#include "SolidSyslogSdValue.h"
#include "SolidSyslogStructuredDataDefinition.h"

/* Server authentication until Init says otherwise: a missing client credential
 * disables mTLS without failing the connection, and claiming protection the device
 * does not have would defeat the point of reporting it at all. */
static const char* s_transport = "tls";

/* Called once per record. A non-zero enterprise number is what makes the SD-ID
 * private — _Begin emits "name@number" for one, a bare IANA "name" for 0. The
 * element writer owns the brackets, the SD-NAME charset and the escaping, so this
 * supplies only names and values and cannot desync the framing. */
static void SyslogPipelineSd_Format(struct SolidSyslogStructuredData* base, struct SolidSyslogSdElement* element)
{
    (void) base;

    SolidSyslogSdElement_Begin(element, "logPipeline", SYSLOG_ENTERPRISE_NUMBER);
    SolidSyslogSdValue_String(SolidSyslogSdElement_Param(element, "transport"), s_transport);
    SolidSyslogSdValue_String(SolidSyslogSdElement_Param(element, "atRest"), "hmac-sha256");
    SolidSyslogSdElement_End(element);
}

/* No _Create and no pool slot: the library never allocates an SD source, so a
 * stateless one is a vtable this application owns. */
static struct SolidSyslogStructuredData s_pipelineSd = {SyslogPipelineSd_Format};

struct SolidSyslogStructuredData* SyslogPipelineSd_Init(bool mutualTls)
{
    s_transport = mutualTls ? "mtls" : "tls";
    return &s_pipelineSd;
}
