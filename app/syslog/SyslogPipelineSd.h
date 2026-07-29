/* A worked example of a private enterprise SD-ELEMENT — the part of RFC 5424
 * structured data that is yours to define. The IANA elements say what any device
 * can say; a private one says what only your product knows. This one reports the
 * protection in force on the log pipeline itself. */
#ifndef APP_SYSLOG_PIPELINE_SD_H
#define APP_SYSLOG_PIPELINE_SD_H

#include <stdbool.h>

struct SolidSyslogStructuredData;

/** Records what the pipeline was configured with and returns the shared instance,
 *  for SolidSyslogConfig.Sd. Never NULL. @p mutualTls must reflect the stream
 *  config, not the intent. */
struct SolidSyslogStructuredData* SyslogPipelineSd_Init(bool mutualTls);

#endif /* APP_SYSLOG_PIPELINE_SD_H */
