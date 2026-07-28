/* SolidSyslog compile-time overrides for this device. Reached via
 * SOLIDSYSLOG_USER_TUNABLES_FILE; anything not set here keeps the library
 * default from SolidSyslogTunablesDefaults.h. */
#ifndef SOLID_SYSLOG_TUNABLES_H
#define SOLID_SYSLOG_TUNABLES_H

/* The longest record this device will emit. The library defaults to 2048, the
 * size RFC 5424 section 6.1 says a receiver should accept; over UDP, RFC 5426
 * section 3.2 guarantees only that 480 will be accepted. This device's records are
 * far shorter, so 256 stays well inside every guarantee and anything longer is
 * truncated rather than dropped. */
#define SOLIDSYSLOG_MAX_MESSAGE_SIZE 256U

/* One sender, so one destination address. The default of 3 suits a device
 * running UDP, plain TCP and TLS at once. */
#define SOLIDSYSLOG_ADDRESS_POOL_SIZE 1U

#endif /* SOLID_SYSLOG_TUNABLES_H */
