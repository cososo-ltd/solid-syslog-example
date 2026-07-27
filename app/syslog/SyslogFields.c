/* See SyslogFields.h. */

#include "SyslogFields.h"

#include "DeviceClock.h"

#include "SolidSyslogHeaderField.h"
#include "SolidSyslogTimestamp.h"

#include "lwip/ip4_addr.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"

#include <stdint.h>
#include <string.h>
#include <time.h>

#define SYSLOG_APP_NAME "solid-syslog-example"

void SyslogFields_Clock(struct SolidSyslogTimestamp* timestamp)
{
    struct tm utc;
    uint32_t microseconds = 0U;

    /* Zeroed means "no usable time": Month == 0 fails the library's validation
     * and the field is emitted as the RFC 5424 nil value. */
    (void) memset(timestamp, 0, sizeof(*timestamp));

    if (!DeviceClock_Now(&utc, &microseconds))
    {
        return;
    }

    timestamp->Year = (uint16_t) (utc.tm_year + 1900);
    timestamp->Month = (uint8_t) (utc.tm_mon + 1);
    timestamp->Day = (uint8_t) utc.tm_mday;
    timestamp->Hour = (uint8_t) utc.tm_hour;
    timestamp->Minute = (uint8_t) utc.tm_min;
    timestamp->Second = (uint8_t) utc.tm_sec;
    timestamp->Microsecond = microseconds;
    timestamp->UtcOffsetMinutes = 0;
}

void SyslogFields_Hostname(struct SolidSyslogHeaderField* field, void* context)
{
    (void) context;

    char address[IP4ADDR_STRLEN_MAX] = {0};

    /* netif state belongs to the lwIP core, so read and format under its lock.
     * ip4addr_ntoa_r, not ip4addr_ntoa: the latter shares one static buffer. */
    LOCK_TCPIP_CORE();
    if (netif_default != NULL)
    {
        (void) ip4addr_ntoa_r(netif_ip4_addr(netif_default), address, (int) sizeof(address));
    }
    UNLOCK_TCPIP_CORE();

    if (address[0] != '\0')
    {
        SolidSyslogHeaderField_PrintUsAscii(field, address, strlen(address));
    }
}

void SyslogFields_AppName(struct SolidSyslogHeaderField* field, void* context)
{
    (void) context;

    SolidSyslogHeaderField_PrintUsAscii(field, SYSLOG_APP_NAME, strlen(SYSLOG_APP_NAME));
}
