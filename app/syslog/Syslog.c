/* See Syslog.h.
 *
 * A TLS stream over lwIP TCP behind a circular buffer: Log enqueues and returns,
 * and the service task drains and sends. The mutex is what makes those two sides
 * safe on different tasks.
 *
 * The record carries meta and timeQuality SD-ELEMENTs alongside the filled-in
 * header fields. Unlike a header field, an SD PARAM has no NILVALUE: an unset one
 * is omitted entirely rather than written as "-". */

#include "Syslog.h"

#include "DeviceCertStore.h"

#include "SolidSyslogBlockStore.h"
#include "SolidSyslogCircularBuffer.h"
#include "SolidSyslogConfig.h"
#include "SolidSyslogMbedTlsHmacSha256Policy.h"
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
#include "SolidSyslogMbedTlsStream.h"
#include "SolidSyslogMetaSd.h"
#include "SolidSyslogOriginSd.h"
#include "SolidSyslogSdValue.h"
#include "SolidSyslogStdAtomicCounter.h"
#include "SolidSyslogStreamSender.h"
#include "SolidSyslogTimeQuality.h"
#include "SolidSyslogTimeQualitySd.h"
#include "SyslogFields.h"

#include "lwip/ip4_addr.h"
#include "lwip/tcpip.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* The collector, reached through QEMU's slirp gateway. A numeric literal keeps
 * the resolver numeric-only — no DNS, so no LWIP_DNS and no DNS resolver
 * component to compile. */
#define SYSLOG_COLLECTOR_HOST "10.0.2.2"
#define SYSLOG_COLLECTOR_PORT ((uint16_t) 6514U)

/* Absorbs records logged while the service task is busy sending. Many devices can
 * reduce this further: the store holds the backlog, so the ring only has to cover
 * a burst. */
#define SYSLOG_BUFFER_RECORDS 4U

/* Blocks live on the volume the device already mounts; each is one "<prefix>NN.log". */
#define SYSLOG_STORE_PREFIX "syslog"
#define SYSLOG_STORE_BLOCKS 4U

/* 32473 is the enterprise number reserved for documentation (RFC 5612). Register
 * your own at https://www.iana.org/assignments/enterprise-numbers/ */
#define SYSLOG_ENTERPRISE_ID "32473"
#define SYSLOG_SOFTWARE "solid-syslog-example"
#define SYSLOG_SW_VERSION "0.1.0"

static struct SolidSyslog* s_logger = NULL;
static uint8_t s_ring[SOLIDSYSLOG_CIRCULAR_BUFFER_RING_BYTES(SYSLOG_BUFFER_RECORDS)];

/* The logger reads these on every record, so they outlive Syslog_Start. */
static struct SolidSyslogStructuredData* s_sd[3];

/* One reading at boot, then free-running on the tick — accurate enough to stamp a
 * record, never synchronised to anything. */
static void SyslogTimeQuality(struct SolidSyslogTimeQuality* timeQuality)
{
    timeQuality->TzKnown = true;
    timeQuality->IsSynced = false;
    timeQuality->SyncAccuracyMicroseconds = SOLIDSYSLOG_SYNC_ACCURACY_OMIT;
}

/* The device's own view of its address, which a relay or NAT between it and the
 * collector would otherwise replace. */
static size_t SyslogOriginIpCount(void* context)
{
    (void) context;

    char address[IP4ADDR_STRLEN_MAX] = {0};

    /* Before the interface has an address there is nothing true to say, and an SD
     * PARAM has no nil value — so emit none. */
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

/* Fetched per seal and per verify, never held by the policy. */
static bool SyslogStoreKey(void* context, uint8_t* keyOut, size_t capacity, size_t* keyLengthOut)
{
    (void) context;

    return DeviceCertStore_StoreKey(keyOut, capacity, keyLengthOut);
}

/* Bounds the connect spin so it yields instead of busy-waiting. */
static void SyslogSleep(int milliseconds)
{
    vTaskDelay(pdMS_TO_TICKS(milliseconds));
}

/* Every lwIP Raw call the stream makes has to happen on the thread that owns
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

    struct SolidSyslogLwipRawTcpStreamConfig tcpConfig = {.Sleep = SyslogSleep};

    /* ServerName is checked against the certificate; "" or NULL would drop the peer
     * identity check and leave only the chain. */
    struct SolidSyslogMbedTlsStreamConfig tlsConfig = {
        .Transport = SolidSyslogLwipRawTcpStream_Create(&tcpConfig),
        .Sleep = SyslogSleep,
        .Rng = DeviceCertStore_Rng(),
        .CaChain = DeviceCertStore_CaChain(),
        .ServerName = SYSLOG_COLLECTOR_HOST,
    };

    /* A numeric resolver to parse the literal and an address slot for it to write
     * into. No EndpointVersion — this collector never moves, so the sender
     * resolves once and pins it. */
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
        .GetSysUpTime = SolidSyslogFreeRtosSysUpTime_Get,
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

    struct SolidSyslogMbedTlsHmacSha256PolicyConfig hmacConfig = {.GetKey = SyslogStoreKey};

    /* One file per block on the volume the device already mounts, oldest discarded
     * when the ceiling is reached — a device that cannot reach its collector should
     * keep the newest evidence, not stop logging. */
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
        /* PROCID stays unset — a bare-metal image has no process. */
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
