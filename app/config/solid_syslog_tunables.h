/* SolidSyslog compile-time overrides for this device. Reached via
 * SOLIDSYSLOG_USER_TUNABLES_FILE; anything not set here keeps the library
 * default from SolidSyslogTunablesDefaults.h. */
#ifndef SOLID_SYSLOG_TUNABLES_H
#define SOLID_SYSLOG_TUNABLES_H

/* The worst case measured here is 345 octets: the structured data at full width
 * with a short message. 400 allows for longer messages on this device. Anything
 * longer is truncated rather than dropped. */
#define SOLIDSYSLOG_MAX_MESSAGE_SIZE 400U

/* One sender, over one stream, to one destination. The defaults suit a device
 * running several transports at once. */
#define SOLIDSYSLOG_ADDRESS_POOL_SIZE 1U
#define SOLIDSYSLOG_TCP_STREAM_POOL_SIZE 1U
#define SOLIDSYSLOG_STREAM_SENDER_POOL_SIZE 1U

#endif /* SOLID_SYSLOG_TUNABLES_H */
