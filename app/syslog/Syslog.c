/* See Syslog.h.
 *
 * A TLS stream over lwIP TCP behind a circular buffer: Log enqueues and returns,
 * and the service task drains and sends. The mutex is what makes those two sides
 * safe on different tasks.
 *
 * Unlike a header field, an SD PARAM has no NILVALUE: an unset one is omitted
 * entirely rather than written as "-". */

#include "Syslog.h"

#include "DeviceCertStore.h"

#include "SolidSyslogBlockStore.h"
#include "SolidSyslogCircularBuffer.h"
#include "SolidSyslogConfig.h"
#include "SolidSyslogEndpoint.h"
#include "SolidSyslogEndpointHost.h"
#include "SolidSyslogFatFsFile.h"
#include "SolidSyslogFileBlockDevice.h"
#include "SolidSyslogFreeRtosMutex.h"
#include "SolidSyslogFreeRtosSysUpTime.h"
#include "SolidSyslogLwipRawAddress.h"
#include "SolidSyslogLwipRawMarshal.h"
#include "SolidSyslogLwipRawResolver.h"
#include "SolidSyslogLwipRawTcpStream.h"
#include "SolidSyslogMbedTlsHandleCredentials.h"
#include "SolidSyslogMbedTlsHmacSha256Policy.h"
#include "SolidSyslogMbedTlsStream.h"
#include "SolidSyslogMetaSd.h"
#include "SolidSyslogOriginSd.h"
#include "SolidSyslogSdValue.h"
#include "SolidSyslogStdAtomicCounter.h"
#include "SolidSyslogStreamSender.h"
#include "SolidSyslogTimeQuality.h"
#include "SolidSyslogTimeQualitySd.h"
#include "SyslogEnterprise.h"
#include "SyslogFields.h"
#include "SyslogPipelineSd.h"

#include "lwip/ip4_addr.h"
#include "lwip/tcpip.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* The collector, reached through QEMU's slirp gateway. A numeric literal keeps
 * the resolver numeric-only - no DNS, so no LWIP_DNS and no DNS resolver
 * component to compile. */
#define SYSLOG_COLLECTOR_HOST "10.0.2.2"
#define SYSLOG_COLLECTOR_PORT ((uint16_t) 6514U)

/* Depth enough to absorb a burst while the sender is busy, without sizing for a
 * backlog the store is there to hold. */
#define SYSLOG_BUFFER_RECORDS 8U

/* One "<prefix>NN.log" per block, on the volume the device already mounts. */
#define SYSLOG_STORE_PREFIX "syslog"
#define SYSLOG_STORE_BLOCKS 4U
#define SYSLOG_STORE_KEY_NAME "log-store"

#define SYSLOG_SOFTWARE "solid-syslog-example"

static struct SolidSyslog* s_logger = NULL;
static uint8_t s_ring[SOLIDSYSLOG_CIRCULAR_BUFFER_RING_BYTES(SYSLOG_BUFFER_RECORDS)];

/* The logger reads these on every record, so they outlive Syslog_Start. */
static struct SolidSyslogStructuredData* s_sd[4];

/* One reading at boot, then free-running on the tick - enough to stamp a record,
 * not synchronisation. RFC 5424 section 7.1.3 forbids syncAccuracy alongside an
 * unsynced clock, so none is written. */
static void SyslogTimeQuality(struct SolidSyslogTimeQuality* timeQuality)
{
    timeQuality->TzKnown = true;
    timeQuality->IsSynced = false;
}

/* The device's own view of its address, which a relay or NAT between it and the
 * collector would otherwise replace. */
static size_t SyslogOriginIpCount(void* context)
{
    (void) context;

    char address[IP4ADDR_STRLEN_MAX] = {0};

    SyslogFields_IpAddress(address, sizeof(address));
    return (address[0] != '\0') ? 1U : 0U;
}

static void SyslogOriginIpAt(struct SolidSyslogSdValue* value, void* context, size_t index)
{
    (void) context;
    (void) index;

    char address[IP4ADDR_STRLEN_MAX] = {0};

    SyslogFields_IpAddress(address, sizeof(address));
    SolidSyslogSdValue_String(value, address);
}

/* Fetched per seal and per verify rather than held, so the key never sits on the
 * policy instance. */
static bool SyslogStoreKey(void* context, uint8_t* keyOut, size_t capacity, size_t* keyLengthOut)
{
    (void) context;

    return DeviceCertStore_SymmetricKey(SYSLOG_STORE_KEY_NAME, keyOut, capacity, keyLengthOut);
}

/* Bounds the connect spin so it yields instead of busy-waiting. */
static void SyslogSleep(int milliseconds)
{
    vTaskDelay(pdMS_TO_TICKS(milliseconds));
}

/* Every lwIP Raw call the stream makes has to happen on the thread that owns
 * the lwIP core. lwipopts.h sets LWIP_TCPIP_CORE_LOCKING, so taking the core
 * lock in the caller's own task is simpler and cheaper than posting to the tcpip
 * mailbox - and unconditionally synchronous, which the marshal contract
 * requires. The lock is recursive and these callbacks never re-marshal, so it
 * cannot deadlock against itself. */
static void LwipCoreLockMarshal(SolidSyslogLwipRawCallback callback, void* context)
{
    LOCK_TCPIP_CORE();
    callback(context);
    UNLOCK_TCPIP_CORE();
}

/* The collector's certificate, by fingerprint. Provisioned at commissioning, so
 * no CA is involved and the site needs no PKI. Two slots, so a renewed
 * certificate's pin can sit beside the current one before the collector switches:
 * either authorises it. Both hold the current pin until a renewal is under way. */
static const char* s_collectorPins[2];

/* Moved whenever the pins change. The sender checks it before every record and
 * reconnects when it has moved, so new pins apply without a restart. */
static uint32_t s_streamVersion = 1U;

/* A pin change can be asked for from any task, but the pins and the version are
 * read on the service task as it connects, so changes queue here and the service
 * task applies them between passes. */
#define SYSLOG_PIN_CHANGES 2U
static StaticQueue_t s_pinChangeQueue;
static uint8_t s_pinChangeStorage[SYSLOG_PIN_CHANGES * sizeof(const char*)];
static QueueHandle_t s_pinChanges;

static uint32_t SyslogStreamVersion(void* context)
{
    (void) context;

    return s_streamVersion;
}

/* Asked at every connection. The pin says which certificate; the name says which
 * peer it was issued to. */
static void CollectorProfile(struct SolidSyslogMbedTlsProfile* profile, void* context)
{
    (void) context;

    profile->ServerName = SYSLOG_COLLECTOR_HOST;
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

    struct SolidSyslogLwipRawTcpStreamConfig tcpConfig = {.Sleep = SyslogSleep};

    s_pinChanges = xQueueCreateStatic(SYSLOG_PIN_CHANGES, sizeof(const char*), s_pinChangeStorage, &s_pinChangeQueue);
    s_collectorPins[0] = DeviceCertStore_CollectorPin();
    s_collectorPins[1] = s_collectorPins[0];
    struct SolidSyslogMbedTlsHandleCredentialsConfig credentialsConfig = {
        .Rng = DeviceCertStore_Rng(),
        .PeerFingerprints = s_collectorPins,
        .PeerFingerprintCount = 2U,
    };

    struct SolidSyslogMbedTlsStreamConfig tlsConfig = {
        .Transport = SolidSyslogLwipRawTcpStream_Create(&tcpConfig),
        .Sleep = SyslogSleep,
        .Rng = DeviceCertStore_Rng(),
        .Credentials = SolidSyslogMbedTlsHandleCredentials_Create(&credentialsConfig),
        .Profile = CollectorProfile,
        .Version = SyslogStreamVersion,
    };

    /* No EndpointVersion - this collector never moves, so the sender resolves
     * once and pins it. */
    struct SolidSyslogStreamSenderConfig senderConfig = {
        .Resolver = SolidSyslogLwipRawResolver_Create(),
        .Stream = SolidSyslogMbedTlsStream_Create(&tlsConfig),
        .Address = SolidSyslogLwipRawAddress_Create(),
        .Endpoint = CollectorEndpoint,
    };
    struct SolidSyslogSender* sender = SolidSyslogStreamSender_Create(&senderConfig);

    /* One counter Increment per record formatted, so a record that never reaches
     * the collector leaves a gap in the sequence rather than no trace at all. */
    struct SolidSyslogMetaSdConfig metaConfig = {
        .Counter = SolidSyslogStdAtomicCounter_Create(),
        .GetSysUpTime = SolidSyslogFreeRtos_GetSysUpTime,
    };
    s_sd[0] = SolidSyslogMetaSd_Create(&metaConfig);
    s_sd[1] = SolidSyslogTimeQualitySd_Create(SyslogTimeQuality);

    struct SolidSyslogOriginSdConfig originConfig = {
        .Software = SYSLOG_SOFTWARE,
        .SwVersion = SYSLOG_SW_VERSION,
        .EnterpriseId = SYSLOG_ENTERPRISE_ID,
        .GetIpCount = SyslogOriginIpCount,
        .GetIpAt = SyslogOriginIpAt,
    };
    s_sd[2] = SolidSyslogOriginSd_Create(&originConfig);
    s_sd[3] = SyslogPipelineSd_Get();

    struct SolidSyslogMbedTlsHmacSha256PolicyConfig hmacConfig = {.GetKey = SyslogStoreKey};

    struct SolidSyslogBlockStoreConfig storeConfig = {
        .BlockDevice = SolidSyslogFileBlockDevice_Create(SolidSyslogFatFsFile_Create(), SYSLOG_STORE_PREFIX, 0U),
        .MaxBlocks = SYSLOG_STORE_BLOCKS,
        .DiscardPolicy = SOLIDSYSLOG_DISCARD_POLICY_OLDEST,
        .SecurityPolicy = SolidSyslogMbedTlsHmacSha256Policy_Create(&hmacConfig),
    };

    struct SolidSyslogConfig config = {
        .Buffer = SolidSyslogCircularBuffer_Create(SolidSyslogFreeRtosMutex_Create(), s_ring, sizeof(s_ring)),
        .Sender = sender,
        .Store = SolidSyslogBlockStore_Create(&storeConfig),
        /* PROCID stays unset - a bare-metal image has no process. */
        .Clock = SyslogFields_Clock,
        .GetHostname = SyslogFields_Hostname,
        .GetAppName = SyslogFields_AppName,
        .Sd = s_sd,
        .SdCount = sizeof(s_sd) / sizeof(s_sd[0]),
    };

    s_logger = SolidSyslog_Create(&config);
}

struct SolidSyslog* Syslog_Handle(void)
{
    return s_logger;
}

/* Never queues NULL, which would read as a retirement. */
bool Syslog_ProvisionNextCollectorPin(const char* pin)
{
    return (pin != NULL) && (xQueueSend(s_pinChanges, &pin, 0U) == pdTRUE);
}

/* A NULL change means retire: the next pin becomes the only one. */
bool Syslog_RetireCollectorPin(void)
{
    const char* retire = NULL;
    return xQueueSend(s_pinChanges, &retire, 0U) == pdTRUE;
}

void Syslog_ApplyPinChanges(void)
{
    const char* pin = NULL;
    while (xQueueReceive(s_pinChanges, &pin, 0U) == pdTRUE)
    {
        if (pin != NULL)
        {
            s_collectorPins[1] = pin;
        }
        else
        {
            s_collectorPins[0] = s_collectorPins[1];
        }
        s_streamVersion++;
    }
}
