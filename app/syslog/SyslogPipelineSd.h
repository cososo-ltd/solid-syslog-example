/* A private enterprise SD-ELEMENT, and a worked example of writing one. RFC 5424
 * reserves this form for definitions of your own. This element reports which
 * transport carried the record and which policy protected it at rest. */
#ifndef APP_SYSLOG_PIPELINE_SD_H
#define APP_SYSLOG_PIPELINE_SD_H

#include <stdbool.h>

struct SolidSyslogStructuredData;

/** The shared instance, for SolidSyslogConfig.Sd. Never NULL. @p mutualTls is
 *  what the device holds, not what it meant to configure. */
struct SolidSyslogStructuredData* SyslogPipelineSd_Init(bool mutualTls);

#endif /* APP_SYSLOG_PIPELINE_SD_H */
