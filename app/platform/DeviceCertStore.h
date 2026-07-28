/* The device's provisioned secrets: its trust anchor, the identity it presents
 * for mutual TLS, and its symmetric keys.
 *
 * A real device keeps these in a secure element, TF-M's Internal Trusted
 * Storage, or protected flash, and never has the private key sitting in a file.
 * mps2-an385 under QEMU has no secure world, so the host stands in — as it does
 * for the wall clock and the disk. Replacing this one file is the whole of what
 * a real integration changes.
 *
 * Handles, not bytes: the store parses once at boot and lends out mbedTLS
 * objects, so what a caller needs is a handle it can hold, never a path or a PEM
 * blob — and the encoding between here and the secure store is nobody else's
 * business. */
#ifndef APP_PLATFORM_DEVICE_CERT_STORE_H
#define APP_PLATFORM_DEVICE_CERT_STORE_H

#include "mbedtls/ctr_drbg.h"
#include "mbedtls/pk.h"
#include "mbedtls/x509_crt.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /** Load and parse the device's credentials. Call once, before anything asks
     *  for a handle. False if any of them is missing or will not parse, which on
     *  a real device means the secure store is not provisioned. */
    bool DeviceCertStore_Load(void);

    /** Trust anchors a peer certificate must chain to. NULL until Load succeeds. */
    struct mbedtls_x509_crt* DeviceCertStore_CaChain(void);

    /** This device's own certificate, presented for mTLS. NULL until Load succeeds. */
    struct mbedtls_x509_crt* DeviceCertStore_ClientChain(void);

    /** The private key matching the client certificate. NULL until Load succeeds. */
    struct mbedtls_pk_context* DeviceCertStore_ClientKey(void);

    /** Seeded DRBG for handshakes. NULL until Load succeeds. */
    struct mbedtls_ctr_drbg_context* DeviceCertStore_Rng(void);

    /** Copies the named provisioned symmetric key out. False if unloaded, the name
     *  is unknown, or @p capacity is too small — the key is never lent by pointer. */
    bool DeviceCertStore_SymmetricKey(const char* name, uint8_t* out, size_t capacity, size_t* length);

#ifdef __cplusplus
}
#endif

#endif /* APP_PLATFORM_DEVICE_CERT_STORE_H */
