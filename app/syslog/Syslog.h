/* The device's SolidSyslog wiring — the one place that knows how the logger is
 * assembled. Everything else in the application just logs. */
#ifndef SYSLOG_H
#define SYSLOG_H

struct SolidSyslog;

/** Build the config and create the logger. Call once at startup, after
 *  SyslogErrorHandler_Install so any fault in here is reported. */
void Syslog_Start(void);

/** The logger, for the tasks that log from it and drain it. */
struct SolidSyslog* Syslog_Handle(void);

#endif /* SYSLOG_H */
