/* A worked example of a private enterprise SD-ELEMENT — the part of RFC 5424
 * structured data that is yours to define.
 *
 * The IANA elements say what any device can say. A private one says what only
 * your product knows. This one reports the protection actually in force on the
 * log pipeline, so a SIEM can confirm a record arrived over TLS and was sealed at
 * rest, and can alert on a device whose pipeline has weakened. */
#ifndef APP_SYSLOG_PIPELINE_SD_H
#define APP_SYSLOG_PIPELINE_SD_H

#include <stdbool.h>

struct SolidSyslogStructuredData;

/** Record what the pipeline was actually configured with and return the shared
 *  instance, for SolidSyslogConfig.Sd. Never NULL. @p mutualTls must reflect the
 *  stream config rather than the intent: reporting protection the device does not
 *  have is worse than reporting none. */
struct SolidSyslogStructuredData* SyslogPipelineSd_Init(bool mutualTls);

#endif /* APP_SYSLOG_PIPELINE_SD_H */
