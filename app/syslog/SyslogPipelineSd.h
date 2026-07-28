/* A worked example of a private enterprise SD-ELEMENT — the part of RFC 5424
 * structured data that is yours to define.
 *
 * The IANA elements say what any device can say. A private one says what only
 * your product knows. This one reports the protection actually in force on the
 * log pipeline, so a SIEM can confirm a record arrived over TLS and was sealed at
 * rest, and can alert on a device whose pipeline has weakened. */
#ifndef APP_SYSLOG_PIPELINE_SD_H
#define APP_SYSLOG_PIPELINE_SD_H

struct SolidSyslogStructuredData;

/** Record what the pipeline was actually configured with and return the shared
 *  instance, for SolidSyslogConfig.Sd. Never NULL. Both values must reflect what
 *  was configured rather than what was intended: reporting protection the device
 *  does not have is worse than reporting none. */
struct SolidSyslogStructuredData* SyslogPipelineSd_Init(const char* transport, const char* atRest);

#endif /* APP_SYSLOG_PIPELINE_SD_H */
