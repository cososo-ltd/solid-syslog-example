/* See Syslog.h.
 *
 * The smallest wiring that delivers: a UDP sender over lwIP, and a passthrough
 * buffer in front of it. Passthrough means Log sends inline on the calling
 * task — no queue, no background drain, nothing to service. That is the
 * cheapest configuration SolidSyslog offers and the one this tag measures.
 *
 * The header fields are deliberately left unset. RFC 5424 defines a NILVALUE
 * for every one of them, so a record carrying "-" for timestamp, hostname,
 * app-name and procid is valid and the collector accepts it. Filling them in is
 * a later step, with a cost of its own. */

#include "Syslog.h"

#include "SolidSyslogConfig.h"
#include "SolidSyslogEndpoint.h"
#include "SolidSyslogEndpointHost.h"
#include "SolidSyslogLwipRawAddress.h"
#include "SolidSyslogLwipRawDatagram.h"
#include "SolidSyslogLwipRawMarshal.h"
#include "SolidSyslogLwipRawResolver.h"
#include "SolidSyslogNullStore.h"
#include "SolidSyslogPassthroughBuffer.h"
#include "SolidSyslogUdpSender.h"

#include "lwip/tcpip.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* The collector, reached through QEMU's slirp gateway. A numeric literal keeps
 * the resolver numeric-only — no DNS, so no LWIP_DNS and no DNS resolver
 * component to compile. */
#define SYSLOG_COLLECTOR_HOST "10.0.2.2"
#define SYSLOG_COLLECTOR_PORT ((uint16_t) 5514U)

static struct SolidSyslog* s_logger = NULL;

/* Every lwIP Raw call the datagram makes has to happen on the thread that owns
 * the lwIP core. lwipopts.h sets LWIP_TCPIP_CORE_LOCKING, so taking the core
 * lock in the caller's own task is simpler and cheaper than posting to the tcpip
 * mailbox — and unconditionally synchronous, which the marshal contract
 * requires. The lock is recursive and these callbacks never re-marshal, so it
 * cannot deadlock against itself. */
static void LwipCoreLockMarshal(SolidSyslogLwipRawCallback callback, void* context)
{
    LOCK_TCPIP_CORE();
    callback(context);
    UNLOCK_TCPIP_CORE();
}

/* Pulled by the sender when it connects, not on every send. Host is a bounded
 * sink rather than a raw buffer, so a destination cannot overrun the field. */
static void CollectorEndpoint(struct SolidSyslogEndpoint* endpoint, void* context)
{
    (void) context;

    SolidSyslogEndpointHost_String(endpoint->Host, SYSLOG_COLLECTOR_HOST, strlen(SYSLOG_COLLECTOR_HOST));
    endpoint->Port = SYSLOG_COLLECTOR_PORT;
}

void Syslog_Start(void)
{
    SolidSyslogLwipRaw_SetMarshal(LwipCoreLockMarshal);

    /* One UDP sender: a numeric resolver to parse the literal, a datagram for
     * the socket, and an address slot for the resolver to write into. No
     * EndpointVersion — this collector never moves, so the sender resolves once
     * and pins it. */
    struct SolidSyslogUdpSenderConfig senderConfig = {
        .Resolver = SolidSyslogLwipRawResolver_Create(),
        .Datagram = SolidSyslogLwipRawDatagram_Create(),
        .Address = SolidSyslogLwipRawAddress_Create(),
        .Endpoint = CollectorEndpoint,
    };
    struct SolidSyslogSender* sender = SolidSyslogUdpSender_Create(&senderConfig);

    struct SolidSyslogConfig config = {
        .Buffer = SolidSyslogPassthroughBuffer_Create(sender),
        .Sender = sender,
        /* This device does no store-and-forward. Passing the Null object rather
         * than NULL is how that is said out loud: NULL means "I forgot" and is
         * reported as a fault, the Null object means "I meant this" and is
         * silent. The two behave identically at run time. */
        .Store = SolidSyslogNullStore_Get(),
    };

    s_logger = SolidSyslog_Create(&config);
}

struct SolidSyslog* Syslog_Handle(void)
{
    return s_logger;
}
