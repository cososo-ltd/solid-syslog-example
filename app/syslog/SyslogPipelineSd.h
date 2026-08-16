/* A private enterprise SD-ELEMENT, and a worked example of writing one. RFC 5424
 * reserves this form for definitions of your own. This element reports which
 * transport carried the record and which policy protected it at rest. */
#ifndef APP_SYSLOG_PIPELINE_SD_H
#define APP_SYSLOG_PIPELINE_SD_H

struct SolidSyslogStructuredData;

/** Records the protection in force and returns the shared instance, for
 *  SolidSyslogConfig.Sd. Never NULL. Both values must reflect what the device
 *  holds, not what it was meant to be configured with. */
struct SolidSyslogStructuredData* SyslogPipelineSd_Init(const char* transport, const char* atRest);

#endif /* APP_SYSLOG_PIPELINE_SD_H */
