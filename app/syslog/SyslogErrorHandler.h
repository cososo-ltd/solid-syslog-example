/* The device's reaction to a SolidSyslog internal fault.
 *
 * SolidSyslog never fails loudly: every _Create degrades to a Null object and
 * every delivery fault is absorbed, so a logger that has silently stopped
 * logging looks exactly like one with nothing to say. The error handler is the
 * only thing that tells them apart, which is why it is installed before the
 * first _Create rather than added once something looks wrong. */
#ifndef SYSLOG_ERROR_HANDLER_H
#define SYSLOG_ERROR_HANDLER_H

/** Install the handler on the library's single global slot. Call once, at
 *  startup, before any SolidSyslog object is created. */
void SyslogErrorHandler_Install(void);

#endif /* SYSLOG_ERROR_HANDLER_H */
