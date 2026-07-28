/* The device's wall clock: a time acquired at boot, carried forward by the
 * FreeRTOS tick. mps2-an385 has no RTC, so the time comes from the host over
 * semihosting - standing in for the NTP or provisioning exchange a real device
 * would do. FatFs stamps files with it. */
#ifndef DEVICE_CLOCK_H
#define DEVICE_CLOCK_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

/** Acquire the time. Call once, after the scheduler starts. */
void DeviceClock_Start(void);

/** Current UTC time. False if it was never acquired, leaving the outputs
 *  untouched. */
bool DeviceClock_Now(struct tm* utc, uint32_t* microseconds);

/** Current UTC time as seconds since the Unix epoch, for callers that want a
 *  scalar rather than a broken-down time. 0 if it was never acquired. */
time_t DeviceClock_UnixTime(void);

#endif /* DEVICE_CLOCK_H */
