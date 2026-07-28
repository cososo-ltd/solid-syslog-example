/* The device's reaction to a SolidSyslog internal fault.
 *
 * Nothing in the library fails loudly — a _Create that cannot succeed returns a
 * Null object instead — so a logger that has silently stopped looks exactly like
 * one with nothing to say. This is the only thing that tells them apart, which
 * is why it goes in before the first _Create and not after something looks
 * wrong. */
#ifndef SYSLOG_ERROR_HANDLER_H
#define SYSLOG_ERROR_HANDLER_H

/** Install the handler on the library's single global slot. Call once, at
 *  startup, before any SolidSyslog object is created. */
void SyslogErrorHandler_Install(void);

#endif /* SYSLOG_ERROR_HANDLER_H */
