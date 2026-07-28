/* SolidSyslog compile-time overrides for this device. Reached via
 * SOLIDSYSLOG_USER_TUNABLES_FILE; anything not set here keeps the library
 * default from SolidSyslogTunablesDefaults.h. */
#ifndef SOLID_SYSLOG_TUNABLES_H
#define SOLID_SYSLOG_TUNABLES_H

/* The longest record this device will emit; anything longer is truncated rather
 * than dropped. Sized to this device's worst-case record, and within what RFC 5424
 * section 6.1 asks a receiver to accept. */
#define SOLIDSYSLOG_MAX_MESSAGE_SIZE 512U

/* One sender, so one destination address. The default of 3 suits a device
 * running UDP, plain TCP and TLS at once. */
#define SOLIDSYSLOG_ADDRESS_POOL_SIZE 1U

#endif /* SOLID_SYSLOG_TUNABLES_H */
