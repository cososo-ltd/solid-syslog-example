/* A private enterprise SD-ELEMENT, and a worked example of writing one. RFC 5424
 * reserves this form for definitions of your own. This element reports which
 * transport carried the record and which policy protected it at rest. */
#ifndef APP_SYSLOG_PIPELINE_SD_H
#define APP_SYSLOG_PIPELINE_SD_H

struct SolidSyslogStructuredData;

/** The shared instance, for SolidSyslogConfig.Sd. Stateless, so never NULL. */
struct SolidSyslogStructuredData* SyslogPipelineSd_Get(void);

#endif /* APP_SYSLOG_PIPELINE_SD_H */
