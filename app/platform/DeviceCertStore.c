/* See DeviceCertStore.h. */

#include "DeviceCertStore.h"

#include "SemihostingIo.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Where the host keeps this device's credentials, relative to QEMU's working
 * directory. scripts/gen-certs.sh writes them there fresh on every run. */
#define CERT_PATH_CA "build/certs/ca.crt"
#define CERT_PATH_CLIENT "build/certs/device.crt"
#define CERT_PATH_CLIENT_KEY "build/certs/device.key"

#define DEVICE_SYMMETRIC_KEY_LENGTH 32U

/* The symmetric keys this device was provisioned with, by name. One slot per key;
 * a real device reads these from its secure element rather than from files. */
static struct
{
    const char* Name;
    const char* Path;
    uint8_t Key[DEVICE_SYMMETRIC_KEY_LENGTH];
} s_symmetricKeys[] = {
    {"device-storage", "build/certs/device-storage.key", {0}},
    {"log-store", "build/certs/log-store.key", {0}},
};

/* One scratch buffer, reused for each file: mbedtls_x509_crt_parse and
 * mbedtls_pk_parse_key copy what they need into their own objects, so nothing
 * here has to outlive the parse. P-256 PEM runs to roughly 750 bytes. */
static char s_scratch[1536];

static mbedtls_x509_crt s_caChain;
static mbedtls_x509_crt s_clientChain;
static mbedtls_pk_context s_clientKey;
static mbedtls_ctr_drbg_context s_rng;
static bool s_loaded;

/* NOT an entropy source. mps2-an385 has no TRNG and QEMU models none, so this
 * stands in for the hardware RNG a real device seeds from — the same standing-in
 * the semihosting clock does for NTP. It is deterministic, and a device that
 * shipped with it would have no security at all. */
static int DeviceCertStore_Entropy(void* context, unsigned char* output, size_t length)
{
    (void) context;
    static uint32_t state = 0x6D2B79F5U;
    for (size_t i = 0; i < length; i++)
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        output[i] = (unsigned char) state;
    }
    return 0;
}

/* PEM is text and mbedTLS wants the terminating NUL counted in the length, which
 * is exactly what SemihostingIo_ReadFile produces. */
static bool DeviceCertStore_ReadPem(const char* path, size_t* length)
{
    size_t bytesRead = 0;
    if (!SemihostingIo_ReadFile(path, s_scratch, sizeof(s_scratch), &bytesRead))
    {
        (void) printf("[sim] cert store: cannot read %s\n", path);
        return false;
    }
    *length = bytesRead + 1U;
    return true;
}

static bool DeviceCertStore_LoadCertificate(const char* path, mbedtls_x509_crt* into)
{
    size_t length = 0;
    if (!DeviceCertStore_ReadPem(path, &length))
    {
        return false;
    }

    int result = mbedtls_x509_crt_parse(into, (const unsigned char*) s_scratch, length);
    if (result != 0)
    {
        (void) printf("[sim] cert store: %s failed to parse (-0x%04x)\n", path, (unsigned) -result);
        return false;
    }
    return true;
}

bool DeviceCertStore_Load(void)
{
    if (s_loaded)
    {
        return true;
    }

    mbedtls_x509_crt_init(&s_caChain);
    mbedtls_x509_crt_init(&s_clientChain);
    mbedtls_pk_init(&s_clientKey);
    mbedtls_ctr_drbg_init(&s_rng);

    /* Seeded first: mbedTLS 3.x takes an RNG for the key parse, for blinding. */
    static const char personalisation[] = "solid-syslog-example device";
    int seeded = mbedtls_ctr_drbg_seed(
        &s_rng, DeviceCertStore_Entropy, NULL, (const unsigned char*) personalisation, sizeof(personalisation) - 1U
    );
    if (seeded != 0)
    {
        (void) printf("[sim] cert store: DRBG seed failed (-0x%04x)\n", (unsigned) -seeded);
        return false;
    }

    if (!DeviceCertStore_LoadCertificate(CERT_PATH_CA, &s_caChain))
    {
        return false;
    }
    if (!DeviceCertStore_LoadCertificate(CERT_PATH_CLIENT, &s_clientChain))
    {
        return false;
    }

    size_t keyLength = 0;
    if (!DeviceCertStore_ReadPem(CERT_PATH_CLIENT_KEY, &keyLength))
    {
        return false;
    }
    int parsed = mbedtls_pk_parse_key(
        &s_clientKey, (const unsigned char*) s_scratch, keyLength, NULL, 0U, mbedtls_ctr_drbg_random, &s_rng
    );
    if (parsed != 0)
    {
        (void) printf("[sim] cert store: client key failed to parse (-0x%04x)\n", (unsigned) -parsed);
        return false;
    }

    for (size_t i = 0; i < (sizeof(s_symmetricKeys) / sizeof(s_symmetricKeys[0])); i++)
    {
        size_t read = 0;
        if (!SemihostingIo_ReadFile(s_symmetricKeys[i].Path, s_scratch, sizeof(s_scratch), &read)
            || (read != DEVICE_SYMMETRIC_KEY_LENGTH))
        {
            (void) printf("[sim] cert store: %s missing or wrong length\n", s_symmetricKeys[i].Path);
            return false;
        }
        (void) memcpy(s_symmetricKeys[i].Key, s_scratch, DEVICE_SYMMETRIC_KEY_LENGTH);
    }

    s_loaded = true;
    return true;
}

struct mbedtls_x509_crt* DeviceCertStore_CaChain(void)
{
    return s_loaded ? &s_caChain : NULL;
}

struct mbedtls_x509_crt* DeviceCertStore_ClientChain(void)
{
    return s_loaded ? &s_clientChain : NULL;
}

struct mbedtls_pk_context* DeviceCertStore_ClientKey(void)
{
    return s_loaded ? &s_clientKey : NULL;
}

struct mbedtls_ctr_drbg_context* DeviceCertStore_Rng(void)
{
    return s_loaded ? &s_rng : NULL;
}

bool DeviceCertStore_SymmetricKey(const char* name, uint8_t* out, size_t capacity, size_t* length)
{
    if (!s_loaded || (capacity < DEVICE_SYMMETRIC_KEY_LENGTH))
    {
        return false;
    }

    for (size_t i = 0; i < (sizeof(s_symmetricKeys) / sizeof(s_symmetricKeys[0])); i++)
    {
        if (strcmp(s_symmetricKeys[i].Name, name) == 0)
        {
            (void) memcpy(out, s_symmetricKeys[i].Key, DEVICE_SYMMETRIC_KEY_LENGTH);
            *length = DEVICE_SYMMETRIC_KEY_LENGTH;
            return true;
        }
    }
    return false;
}
