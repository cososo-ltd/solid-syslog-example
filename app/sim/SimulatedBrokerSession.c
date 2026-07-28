/* See SimulatedBrokerSession.h. */

#include "SimulatedBrokerSession.h"

#include "DeviceCertStore.h"

#include "lwip/ip_addr.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/tcpip.h"

#include "mbedtls/ctr_drbg.h"
#include "mbedtls/ssl.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stdint.h>
#include <stdio.h>

/* The broker, on the IANA secure-MQTT port, reached through QEMU's slirp
 * gateway. It has to match a SAN in the broker certificate or the hostname
 * check below rejects it. */
#define BROKER_HOST "10.0.2.2"
#define BROKER_PORT ((uint16_t) 8883U)

/* Bounded throughout, so a broker that never answers fails the bring-up rather
 * than hanging it. One poll is one tick at this device's tick rate. */
#define BROKER_POLL_MS 10U
#define BROKER_CONNECT_TIMEOUT_MS 5000U
#define BROKER_HANDSHAKE_TIMEOUT_MS 15000U
#define BROKER_REPLY_TIMEOUT_MS 5000U

/* One exchange, because a completed handshake does not show that the session
 * carries traffic. The far end echoes whatever it is sent. */
static const char BROKER_HELLO[] = "device-online\n";

/* The TCP half. lwIP's raw API is callback-driven and belongs to one thread, so
 * every call into it below is made under the core lock, and the callbacks touch
 * nothing else. */
static struct tcp_pcb* s_pcb;
static struct pbuf* s_receiveQueue;
static uint16_t s_receiveOffset;
static volatile bool s_connected;
static volatile bool s_errored;

static mbedtls_ssl_context s_ssl;
static mbedtls_ssl_config s_sslConfig;

static bool BrokerSession_Failed(const char* what, int code)
{
    (void) printf("[sim] broker: %s failed (-0x%04x)\n", what, (unsigned) -code);
    return false;
}

static err_t BrokerSession_OnConnected(void* argument, struct tcp_pcb* pcb, err_t error)
{
    (void) argument;
    (void) pcb;

    if (error == ERR_OK)
    {
        s_connected = true;
    }
    else
    {
        s_errored = true;
    }
    return ERR_OK;
}

/* A NULL pbuf is the peer's FIN. Anything else joins the tail of what is already
 * queued; the read cursor is tracked separately, so appending to a partly-read
 * chain is safe. */
static err_t BrokerSession_OnReceive(void* argument, struct tcp_pcb* pcb, struct pbuf* received, err_t error)
{
    (void) argument;
    (void) pcb;
    (void) error;

    if (received == NULL)
    {
        s_errored = true;
    }
    else if (s_receiveQueue == NULL)
    {
        s_receiveQueue = received;
    }
    else
    {
        pbuf_cat(s_receiveQueue, received);
    }
    return ERR_OK;
}

/* lwIP has already released the pcb by the time this runs, so the pointer must
 * be dropped rather than closed. */
static void BrokerSession_OnError(void* argument, err_t error)
{
    (void) argument;
    (void) error;

    s_pcb = NULL;
    s_errored = true;
}

/* Waits on the calling task, never on the tcpip thread — sleeping there would
 * starve the RX and timer paths the connect needs to make progress. */
static bool BrokerSession_WaitConnected(void)
{
    uint32_t elapsedMs = 0;
    while (!s_connected && !s_errored && (elapsedMs < BROKER_CONNECT_TIMEOUT_MS))
    {
        vTaskDelay(pdMS_TO_TICKS(BROKER_POLL_MS));
        elapsedMs += BROKER_POLL_MS;
    }
    return s_connected;
}

static void BrokerSession_AbortTcp(void)
{
    LOCK_TCPIP_CORE();
    if (s_pcb != NULL)
    {
        tcp_abort(s_pcb);
        s_pcb = NULL;
    }
    UNLOCK_TCPIP_CORE();
}

static bool BrokerSession_OpenTcp(void)
{
    ip_addr_t address;
    if (!ipaddr_aton(BROKER_HOST, &address))
    {
        (void) printf("[sim] broker: %s is not an address\n", BROKER_HOST);
        return false;
    }

    err_t connectError = ERR_MEM;
    LOCK_TCPIP_CORE();
    s_pcb = tcp_new();
    if (s_pcb != NULL)
    {
        /* With Nagle on, a sub-MSS handshake flight is held until the previous
         * segment is acked — and the peer only acks once it has the whole
         * flight. The two wait for each other. */
        tcp_nagle_disable(s_pcb);
        tcp_recv(s_pcb, BrokerSession_OnReceive);
        tcp_err(s_pcb, BrokerSession_OnError);
        connectError = tcp_connect(s_pcb, &address, BROKER_PORT, BrokerSession_OnConnected);
    }
    UNLOCK_TCPIP_CORE();

    bool ok = (connectError == ERR_OK) && BrokerSession_WaitConnected();
    if (!ok)
    {
        BrokerSession_AbortTcp();
        (void) printf("[sim] broker: no TCP connection to %s:%u\n", BROKER_HOST, (unsigned) BROKER_PORT);
    }
    return ok;
}

static int BrokerSession_Send(void* context, const unsigned char* buffer, size_t length)
{
    (void) context;

    int result = -1;
    LOCK_TCPIP_CORE();
    if (s_pcb != NULL)
    {
        err_t writeError = tcp_write(s_pcb, buffer, (u16_t) length, TCP_WRITE_FLAG_COPY);
        if (writeError == ERR_OK)
        {
            /* ERR_MEM from tcp_output means lwIP has the bytes and will retry. */
            err_t outputError = tcp_output(s_pcb);
            result = ((outputError == ERR_OK) || (outputError == ERR_MEM)) ? (int) length : -1;
        }
        else if (writeError == ERR_MEM)
        {
            /* The send buffer is full. Retryable, not fatal — tearing the session
             * down over a full window would be a self-inflicted disconnect. */
            result = MBEDTLS_ERR_SSL_WANT_WRITE;
        }
        else
        {
            /* Keep result = -1: a real write failure. */
        }
    }
    UNLOCK_TCPIP_CORE();
    return result;
}

/* mbedTLS reads through a non-blocking transport, so "nothing yet" has to be
 * WANT_READ rather than 0 — a 0 would end the handshake on the first poll. */
static int BrokerSession_Receive(void* context, unsigned char* buffer, size_t length)
{
    (void) context;

    int result;
    LOCK_TCPIP_CORE();
    if (s_receiveQueue != NULL)
    {
        u16_t available = (u16_t) (s_receiveQueue->tot_len - s_receiveOffset);
        u16_t wanted = (length < (size_t) available) ? (u16_t) length : available;
        u16_t copied = pbuf_copy_partial(s_receiveQueue, buffer, wanted, s_receiveOffset);

        s_receiveOffset = (uint16_t) (s_receiveOffset + copied);
        if (s_receiveOffset >= s_receiveQueue->tot_len)
        {
            (void) pbuf_free(s_receiveQueue);
            s_receiveQueue = NULL;
            s_receiveOffset = 0;
        }
        if (s_pcb != NULL)
        {
            /* Reopen the window by what was consumed, not by what arrived. */
            tcp_recved(s_pcb, copied);
        }
        result = (int) copied;
    }
    else
    {
        result = s_errored ? -1 : MBEDTLS_ERR_SSL_WANT_READ;
    }
    UNLOCK_TCPIP_CORE();
    return result;
}

static bool BrokerSession_Configure(void)
{
    mbedtls_ssl_init(&s_ssl);
    mbedtls_ssl_config_init(&s_sslConfig);

    int result = mbedtls_ssl_config_defaults(
        &s_sslConfig,
        MBEDTLS_SSL_IS_CLIENT,
        MBEDTLS_SSL_TRANSPORT_STREAM,
        MBEDTLS_SSL_PRESET_DEFAULT
    );
    if (result != 0)
    {
        return BrokerSession_Failed("ssl defaults", result);
    }

    /* The device authenticates the broker, and presents its own certificate so
     * the broker can authenticate the device. Either half missing is not mTLS. */
    mbedtls_ssl_conf_authmode(&s_sslConfig, MBEDTLS_SSL_VERIFY_REQUIRED);
    mbedtls_ssl_conf_ca_chain(&s_sslConfig, DeviceCertStore_CaChain(), NULL);
    mbedtls_ssl_conf_rng(&s_sslConfig, mbedtls_ctr_drbg_random, DeviceCertStore_Rng());

    result = mbedtls_ssl_conf_own_cert(&s_sslConfig, DeviceCertStore_ClientChain(), DeviceCertStore_ClientKey());
    if (result != 0)
    {
        return BrokerSession_Failed("client certificate", result);
    }

    result = mbedtls_ssl_setup(&s_ssl, &s_sslConfig);
    if (result != 0)
    {
        return BrokerSession_Failed("ssl setup", result);
    }

    result = mbedtls_ssl_set_hostname(&s_ssl, BROKER_HOST);
    if (result != 0)
    {
        return BrokerSession_Failed("expected hostname", result);
    }

    mbedtls_ssl_set_bio(&s_ssl, NULL, BrokerSession_Send, BrokerSession_Receive, NULL);
    return true;
}

static bool BrokerSession_IsRetryable(int result)
{
    return (result == MBEDTLS_ERR_SSL_WANT_READ) || (result == MBEDTLS_ERR_SSL_WANT_WRITE);
}

static bool BrokerSession_Handshake(void)
{
    uint32_t elapsedMs = 0;
    int result = mbedtls_ssl_handshake(&s_ssl);
    while (BrokerSession_IsRetryable(result) && (elapsedMs < BROKER_HANDSHAKE_TIMEOUT_MS))
    {
        vTaskDelay(pdMS_TO_TICKS(BROKER_POLL_MS));
        elapsedMs += BROKER_POLL_MS;
        result = mbedtls_ssl_handshake(&s_ssl);
    }
    if (result != 0)
    {
        return BrokerSession_Failed("handshake", result);
    }

    /* VERIFY_REQUIRED has already failed the handshake on a bad chain; asking
     * again is what turns "it connected" into a reason it was allowed to. */
    uint32_t verdict = mbedtls_ssl_get_verify_result(&s_ssl);
    if (verdict != 0)
    {
        (void) printf("[sim] broker: peer certificate rejected (0x%08lx)\n", (unsigned long) verdict);
        return false;
    }
    return true;
}

static bool BrokerSession_Exchange(void)
{
    uint32_t elapsedMs = 0;
    int result = mbedtls_ssl_write(&s_ssl, (const unsigned char*) BROKER_HELLO, sizeof(BROKER_HELLO) - 1U);
    while (BrokerSession_IsRetryable(result) && (elapsedMs < BROKER_REPLY_TIMEOUT_MS))
    {
        vTaskDelay(pdMS_TO_TICKS(BROKER_POLL_MS));
        elapsedMs += BROKER_POLL_MS;
        result = mbedtls_ssl_write(&s_ssl, (const unsigned char*) BROKER_HELLO, sizeof(BROKER_HELLO) - 1U);
    }
    if (result <= 0)
    {
        return BrokerSession_Failed("send", result);
    }

    /* What comes back is not inspected — what the broker answers is proved in
     * scripts/smoke-oracle.sh, before the device runs. Here it only has to
     * arrive, over the session, from the peer that authenticated. */
    unsigned char reply[32];
    elapsedMs = 0;
    result = mbedtls_ssl_read(&s_ssl, reply, sizeof(reply));
    while (BrokerSession_IsRetryable(result) && (elapsedMs < BROKER_REPLY_TIMEOUT_MS))
    {
        vTaskDelay(pdMS_TO_TICKS(BROKER_POLL_MS));
        elapsedMs += BROKER_POLL_MS;
        result = mbedtls_ssl_read(&s_ssl, reply, sizeof(reply));
    }
    if (result <= 0)
    {
        return BrokerSession_Failed("reply", result);
    }
    return true;
}

bool SimulatedBrokerSession_Open(void)
{
    /* Nothing closes it. The session is held for the life of the device, which
     * is what makes it concurrent with SolidSyslog's rather than sequential —
     * two sessions that never overlap would measure the larger, not the sum. */
    bool ok = BrokerSession_OpenTcp() && BrokerSession_Configure() && BrokerSession_Handshake()
              && BrokerSession_Exchange();
    if (ok)
    {
        (void) printf(
            "[sim] broker session to %s:%u: %s, %s\n",
            BROKER_HOST,
            (unsigned) BROKER_PORT,
            mbedtls_ssl_get_version(&s_ssl),
            mbedtls_ssl_get_ciphersuite(&s_ssl)
        );
    }
    return ok;
}
