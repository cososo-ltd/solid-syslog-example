/* The device's SolidSyslog wiring - the one place that knows how the logger is
 * assembled. Everything else in the application just logs. */
#ifndef SYSLOG_H
#define SYSLOG_H

#include <stdbool.h>

struct SolidSyslog;

/** Build the config and create the logger. Call once at startup, after
 *  SyslogErrorHandler_Install so any fault in here is reported. */
void Syslog_Start(void);

/** The logger, for the tasks that log from it and drain it. */
struct SolidSyslog* Syslog_Handle(void);

/** Accept a renewed collector certificate beside the current one, ahead of the
 *  collector switching to it. Callable from any task; the change is applied on
 *  the service task. The string must outlive every connection that uses it.
 *  False if pin is NULL or the change could not be queued: nothing changed. */
bool Syslog_ProvisionNextCollectorPin(const char* pin);

/** Stop accepting the collector's previous certificate, once it has renewed.
 *  Callable from any task; the change is applied on the service task.
 *  False if the change could not be queued: nothing changed, so ask again. */
bool Syslog_RetireCollectorPin(void);

/** Apply the pin changes asked for since the last call. Service task only. */
void Syslog_ApplyPinChanges(void);

#endif /* SYSLOG_H */
