/* The simulated existing application — see SimulatedExistingApp.h.
 *
 * This is the "device you already have": networked, with storage, and holding a
 * mutual-TLS session to its broker. lwIP, FatFs and that session are all brought
 * up here, so their runtime cost is baseline. What is left of the platform
 * surface SolidSyslog will call — datagrams, files, the teardown path a
 * held-open session never runs — is rooted into the image below without being
 * run, so its flash is counted below the line too and a later tag's delta is
 * only the SolidSyslog code that drives it. */

#include "SimulatedExistingApp.h"

#include "AppConfig.h"
#include "DeviceCertStore.h"
#include "EthernetIf.h"
#include "SimulatedBrokerSession.h"

#include "lwip/ip4_addr.h"
#include "lwip/netif.h"
#include "lwip/tcp.h"
#include "lwip/tcpip.h"
#include "lwip/udp.h"

#include "ff.h"

#include "mbedtls/memory_buffer_alloc.h"
#include "mbedtls/ssl.h"
#include "psa/crypto.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* Everything mbedTLS allocates comes out of this, and nothing else uses it.
 * Sized in AppConfig.h, from what this measures. */
static uint8_t s_mbedtlsHeap[SIMULATED_APP_MBEDTLS_HEAP_BYTES];

size_t SimulatedExistingApp_MbedTlsPeak(void)
{
    size_t maxUsed = 0;
    size_t maxBlocks = 0;

    mbedtls_memory_buffer_alloc_max_get(&maxUsed, &maxBlocks);
    return maxUsed;
}

/* lwIP randomness source (referenced by arch/cc.h's LWIP_RAND for TCP ISN
 * selection). Self-contained xorshift32 — no entropy backend in the baseline. */
unsigned int LwipPortRand(void)
{
    static uint32_t state = 0x2545F491U;
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return (unsigned int) state;
}

/* Present because mbedtls_user_config.h sets MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG
 * (mbedTLS 3.6's TLS 1.3 path is PSA-based). Linked so the PSA surface resolves;
 * not exercised at Baseline. A real integrator wires this to a hardware RNG. */
psa_status_t mbedtls_psa_external_get_random(
    mbedtls_psa_external_random_context_t* context,
    uint8_t* output,
    size_t output_size,
    size_t* output_length
)
{
    (void) context;
    for (size_t i = 0; i < output_size; i++)
    {
        output[i] = (uint8_t) (i * 2654435761U);
    }
    *output_length = output_size;
    return PSA_SUCCESS;
}

/* ---- keep-alive: root the platform surface without running it --------------
 *
 * Taking a function's address roots its code section, and its body roots
 * everything it calls — so referencing these top-level entry points drags their
 * whole static call closures into the image under --gc-sections. The addresses
 * are XOR'd into a volatile sink reached from main(), which keeps this reachable
 * (belt-and-braces over the `used` attribute). Nothing here is called.
 *
 * Only what the device does not itself run belongs in this list. The broker
 * session calls the mbedTLS client surface and lwIP's TCP path for real, so
 * those root themselves and naming them here would say something untrue. */
static volatile uintptr_t s_keepAliveSink;

__attribute__((used)) static void KeepPlatformLinked(void)
{
    static const uintptr_t surface[] = {
        /* lwIP raw UDP — the SolidSyslog UdpSender (Minimal). The device's own
         * traffic is TCP, so nothing else reaches these. */
        (uintptr_t) &udp_new,
        (uintptr_t) &udp_bind,
        (uintptr_t) &udp_connect,
        (uintptr_t) &udp_sendto,
        (uintptr_t) &udp_recv,
        (uintptr_t) &udp_remove,
        /* lwIP raw TCP, the parts a session that is never closed does not use. */
        (uintptr_t) &tcp_bind,
        (uintptr_t) &tcp_sent,
        (uintptr_t) &tcp_close,
        /* FatFs file API — the SolidSyslog BlockStore (Secure). */
        (uintptr_t) &f_open,
        (uintptr_t) &f_close,
        (uintptr_t) &f_read,
        (uintptr_t) &f_write,
        (uintptr_t) &f_sync,
        (uintptr_t) &f_lseek,
        (uintptr_t) &f_truncate,
        (uintptr_t) &f_unlink,
        (uintptr_t) &f_stat,
        /* The mbedTLS teardown path. A device that reconnects to its broker runs
         * it; this one holds one session for its whole life and never does, so
         * without this the cost would land on whoever closes first. */
        (uintptr_t) &mbedtls_ssl_close_notify,
        (uintptr_t) &mbedtls_ssl_session_reset,
        (uintptr_t) &mbedtls_ssl_free,
        (uintptr_t) &mbedtls_ssl_config_free,
        /* The provisioned symmetric key lookup. Nothing in the simulated device
         * reads a key, so without this the accessor is stripped and the cost
         * reappears on whoever first asks for one. */
        (uintptr_t) &DeviceCertStore_SymmetricKey,
    };

    uintptr_t sink = 0;
    for (size_t i = 0; i < (sizeof(surface) / sizeof(surface[0])); i++)
    {
        sink ^= surface[i];
    }
    s_keepAliveSink = sink;
}

/* ---- lwIP netif bring-up (runs on the tcpip thread) ------------------------ */

static struct netif s_interface;
static SemaphoreHandle_t s_netifReady;
static bool s_netifUp;

/* Safety ceiling only — real completion is signalled the moment the callback
 * returns. netif_add runs EthernetIf_Init, whose smsc9220_init has its own
 * vTaskDelay-based settle, so give it generous room. */
#define NETIF_BRINGUP_TIMEOUT_MS 5000U

static void SimulatedExistingApp_BringUpNetif(void* context)
{
    (void) context;
    ip4_addr_t ipAddress;
    ip4_addr_t netmask;
    ip4_addr_t gateway;
    IP4_ADDR(&ipAddress, 10, 0, 2, 15);
    IP4_ADDR(&netmask, 255, 255, 255, 0);
    IP4_ADDR(&gateway, 10, 0, 2, 2);

    struct netif* added = netif_add(&s_interface, &ipAddress, &netmask, &gateway, NULL, EthernetIf_Init, tcpip_input);
    if (added != NULL)
    {
        netif_set_default(&s_interface);
        netif_set_up(&s_interface);
        netif_set_link_up(&s_interface);
    }
    s_netifUp = (added != NULL);
    (void) xSemaphoreGive(s_netifReady);
}

/* Bring the netif up on the tcpip thread and BLOCK until it has actually
 * finished — not a fixed delay — propagating a bring-up failure. */
static bool SimulatedExistingApp_NetifUp(void)
{
    static StaticSemaphore_t netifReadyBuffer;

    s_netifReady = xSemaphoreCreateBinaryStatic(&netifReadyBuffer);
    bool ready = false;
    if (tcpip_callback(SimulatedExistingApp_BringUpNetif, NULL) == ERR_OK)
    {
        ready = (xSemaphoreTake(s_netifReady, pdMS_TO_TICKS(NETIF_BRINGUP_TIMEOUT_MS)) == pdTRUE) && s_netifUp;
    }
    return ready;
}

/* ---- FatFs mount (runs on the calling task) -------------------------------- */

static FATFS s_fatFs;

static bool SimulatedExistingApp_MountFatFs(void)
{
    FRESULT result = f_mount(&s_fatFs, "", 1);
    if (result == FR_NO_FILESYSTEM)
    {
        static BYTE workBuffer[FF_MAX_SS];
        const MKFS_PARM options = {.fmt = FM_FAT | FM_SFD, .n_fat = 1, .align = 1, .n_root = 0, .au_size = 0};
        result = f_mkfs("", &options, workBuffer, sizeof(workBuffer));
        if (result == FR_OK)
        {
            result = f_mount(&s_fatFs, "", 1);
        }
    }
    if (result != FR_OK)
    {
        (void) printf("[sim] FatFs mount failed: FRESULT=%d\n", (int) result);
        return false;
    }
    return true;
}

bool SimulatedExistingApp_StartCrypto(void)
{
    /* Before anything mbedTLS allocates, or the allocation lands on newlib's heap
     * and no figure this repository publishes ever sees it. */
    mbedtls_memory_buffer_alloc_init(s_mbedtlsHeap, sizeof(s_mbedtlsHeap));

    /* 3.6's TLS 1.3 path is built on PSA, so no handshake succeeds until this has.
     * Process-global, and the library adapters deliberately never call it. */
    const psa_status_t psaReady = psa_crypto_init();
    if (psaReady != PSA_SUCCESS)
    {
        (void) printf("[sim] PSA crypto init failed: %d\n", (int) psaReady);
        return false;
    }

    if (!DeviceCertStore_Load())
    {
        (void) printf("[sim] cert store unavailable\n");
        return false;
    }
    return true;
}

bool SimulatedExistingApp_Start(void)
{
    /* The rest of the platform surface: linked, never run. */
    KeepPlatformLinked();

    /* lwIP: bring the netif up on the tcpip thread and wait for it to complete. */
    if (!SimulatedExistingApp_NetifUp())
    {
        (void) printf("[sim] netif bring-up failed\n");
        return false;
    }

    /* FatFs: mount (format a fresh image on first use). */
    if (!SimulatedExistingApp_MountFatFs())
    {
        return false;
    }

    /* The broker session, once there is a network to open it on. It stays open
     * from here to the end of the run. */
    return SimulatedBrokerSession_Open();
}
