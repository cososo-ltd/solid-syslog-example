/* This product's IANA Private Enterprise Number, in the two forms RFC 5424 wants
 * it: the number that makes a private SD-ID private, and the string origin's
 * enterpriseId PARAM carries. Defined once and derived, so the two cannot drift.
 *
 * 32473 is reserved for documentation (RFC 5612). Register your own at
 * https://www.iana.org/assignments/enterprise-numbers/ */
#ifndef APP_SYSLOG_ENTERPRISE_H
#define APP_SYSLOG_ENTERPRISE_H

#define SYSLOG_ENTERPRISE_NUMBER 32473

#define SYSLOG_ENTERPRISE_STRINGIFY_(value) #value
#define SYSLOG_ENTERPRISE_STRINGIFY(value) SYSLOG_ENTERPRISE_STRINGIFY_(value)
#define SYSLOG_ENTERPRISE_ID SYSLOG_ENTERPRISE_STRINGIFY(SYSLOG_ENTERPRISE_NUMBER)

#endif /* APP_SYSLOG_ENTERPRISE_H */
