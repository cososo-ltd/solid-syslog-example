/* The RFC 5424 header fields this device supplies: adapters between what the
 * device already has — a wall clock, an IP address, a name — and the shapes
 * SolidSyslog asks for. */
#ifndef SYSLOG_FIELDS_H
#define SYSLOG_FIELDS_H

#include <stddef.h>

struct SolidSyslogTimestamp;
struct SolidSyslogHeaderField;

/** SolidSyslogClockFunction. */
void SyslogFields_Clock(struct SolidSyslogTimestamp* timestamp);

/** The default interface's IPv4 address, or an empty string when unavailable. */
void SyslogFields_IpAddress(char* out, size_t size);

/** HOSTNAME as the interface's IPv4 address — RFC 5424 section 6.2.4 allows an
 *  address where a device has no resolvable name. */
void SyslogFields_Hostname(struct SolidSyslogHeaderField* field, void* context);

/** APP-NAME, fixed for this firmware. */
void SyslogFields_AppName(struct SolidSyslogHeaderField* field, void* context);

#endif /* SYSLOG_FIELDS_H */
