/* The simulated existing application — see SimulatedExistingApp.h.
 *
 * This is the "device you already have": networked, with storage, with a TLS
 * library on board. lwIP and FatFs are brought up (their runtime cost is
 * baseline); mbedTLS is linked but holds no session (SolidSyslog opens the
 * secure channel from Secure). The lwIP raw UDP/TCP API, the FatFs file API, and the
 * mbedTLS client surface that SolidSyslog will call are all rooted into the
 * image here — linked, never run — so their flash is counted below the line and
 * a later tag's delta is only the SolidSyslog code that drives them. */

#include "SimulatedExistingApp.h"

#include "EthernetIf.h"

#include "lwip/ip4_addr.h"
#include "lwip/netif.h"
#include "lwip/tcp.h"
#include "lwip/tcpip.h"
#include "lwip/udp.h"

#include "ff.h"

#include "mbedtls/ctr_drbg.h"
#include "mbedtls/pk.h"
#include "mbedtls/ssl.h"
#include "mbedtls/x509_crt.h"
#include "psa/crypto.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

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
 * (belt-and-braces over the `used` attribute). Nothing is called. */
static volatile uintptr_t s_keepAliveSink;

__attribute__((used)) static void KeepPlatformLinked(void)
{
    static const uintptr_t surface[] = {
        /* lwIP raw UDP + TCP — the SolidSyslog UdpSender (Minimal) / StreamSender (Secure). */
        (uintptr_t) &udp_new,
        (uintptr_t) &udp_bind,
        (uintptr_t) &udp_connect,
        (uintptr_t) &udp_sendto,
        (uintptr_t) &udp_recv,
        (uintptr_t) &udp_remove,
        (uintptr_t) &tcp_new,
        (uintptr_t) &tcp_bind,
        (uintptr_t) &tcp_connect,
        (uintptr_t) &tcp_write,
        (uintptr_t) &tcp_output,
        (uintptr_t) &tcp_recv,
        (uintptr_t) &tcp_sent,
        (uintptr_t) &tcp_close,
        (uintptr_t) &tcp_abort,
        /* FatFs file API — the SolidSyslog BlockStore (Secure). */
        (uintptr_t) &f_open,
        (uintptr_t) &f_close,
        (uintptr_t) &f_read,
        (uintptr_t) &f_write,
        (uintptr_t) &f_sync,
        (uintptr_t) &f_lseek,
        (uintptr_t) &f_unlink,
        (uintptr_t) &f_stat,
        /* mbedTLS client + x509 + pk + DRBG + PSA — the SolidSyslog TLS transport (Secure/Hardened). */
        (uintptr_t) &mbedtls_ssl_setup,
        (uintptr_t) &mbedtls_ssl_handshake,
        (uintptr_t) &mbedtls_ssl_read,
        (uintptr_t) &mbedtls_ssl_write,
        (uintptr_t) &mbedtls_ssl_close_notify,
        (uintptr_t) &mbedtls_ssl_config_defaults,
        (uintptr_t) &mbedtls_ssl_conf_authmode,
        (uintptr_t) &mbedtls_ssl_conf_ca_chain,
        (uintptr_t) &mbedtls_ssl_conf_rng,
        (uintptr_t) &mbedtls_ssl_set_bio,
        (uintptr_t) &mbedtls_ssl_set_hostname,
        (uintptr_t) &mbedtls_x509_crt_parse,
        (uintptr_t) &mbedtls_pk_parse_key,
        (uintptr_t) &mbedtls_ctr_drbg_seed,
        (uintptr_t) &mbedtls_ctr_drbg_random,
        (uintptr_t) &psa_crypto_init,
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
    s_netifReady = xSemaphoreCreateBinary();
    if (s_netifReady == NULL)
    {
        return false;
    }
    bool ready = false;
    if (tcpip_callback(SimulatedExistingApp_BringUpNetif, NULL) == ERR_OK)
    {
        ready = (xSemaphoreTake(s_netifReady, pdMS_TO_TICKS(NETIF_BRINGUP_TIMEOUT_MS)) == pdTRUE) && s_netifUp;
    }
    vSemaphoreDelete(s_netifReady);
    s_netifReady = NULL;
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

bool SimulatedExistingApp_Start(void)
{
    /* mbedTLS + the lwIP raw / FatFs file APIs: linked, never run. */
    KeepPlatformLinked();

    /* lwIP: bring the netif up on the tcpip thread and wait for it to complete. */
    if (!SimulatedExistingApp_NetifUp())
    {
        (void) printf("[sim] netif bring-up failed\n");
        return false;
    }

    /* FatFs: mount (format a fresh image on first use). */
    return SimulatedExistingApp_MountFatFs();
}
