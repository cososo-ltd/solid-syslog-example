/* mbedTLS integrator overrides for this device, layered over mbedTLS's own
 * defaults via -DMBEDTLS_USER_CONFIG_FILE. A #define here adds to the default;
 * an #undef removes from it. Everything untouched keeps mbedTLS's default,
 * cipher suites included - trimming that is a binary-size exercise this device
 * has not needed.
 *
 * Most of what follows removes a host assumption: mbedTLS's generic defaults
 * reach for /dev/urandom, fopen and BSD sockets, none of which a Cortex-M3 has.
 * Entropy, transport and credentials are injected instead. */

#ifndef APP_CONFIG_MBEDTLS_USER_CONFIG_H
#define APP_CONFIG_MBEDTLS_USER_CONFIG_H

/* entropy_poll.c #errors on "Platform entropy sources only work on Unix and
 * Windows" if this is left on. */
#define MBEDTLS_NO_PLATFORM_ENTROPY

/* No filesystem and no sockets: the device reads its own credentials and drives
 * its own transport. */
#undef MBEDTLS_FS_IO
#undef MBEDTLS_NET_C

/* Certificate validity dates are checked only where the build carries a clock.
 * A Cortex-M3 has no syscall behind time() and no monotonic clock_gettime, so
 * both are the device's to supply; gmtime_r comes from newlib. */
#define MBEDTLS_PLATFORM_TIME_ALT
#define MBEDTLS_PLATFORM_MS_TIME_ALT

/* No threading hooks: one task at a time owns an ssl_context here. */
#undef MBEDTLS_THREADING_C
#undef MBEDTLS_THREADING_PTHREAD

/* PSA's trusted storage needs FS_IO, disabled above. */
#undef MBEDTLS_PSA_ITS_FILE_C
#undef MBEDTLS_PSA_CRYPTO_STORAGE_C

/* timing.c is gettimeofday / clock_gettime; the handshake budget is the
 * caller's own bounded retry instead. */
#undef MBEDTLS_TIMING_C

/* mbedTLS sub-allocates from one static buffer the device hands it rather than
 * from a heap, so what it costs is a number chosen up front instead of one
 * observed afterwards, and it cannot fail on a heap someone else fragmented.
 * MEMORY_DEBUG is what makes the number defensible rather than asserted:
 * mbedtls_memory_buffer_alloc_max_get is where the reported peak comes from,
 * and the buffer is sized from it. PLATFORM_MEMORY is its prerequisite. */
#define MBEDTLS_PLATFORM_MEMORY
#define MBEDTLS_MEMORY_BUFFER_ALLOC_C
#define MBEDTLS_MEMORY_DEBUG

/* 3.6's TLS 1.3 path is built on PSA, so psa_crypto_init() has to succeed
 * before any handshake - and PSA's own collector returns
 * PSA_ERROR_INSUFFICIENT_ENTROPY on a platform with no entropy source, which is
 * this one. With this, PSA never seeds itself and takes randomness from the
 * device's callback, so PSA and the classic API share one chain. */
#define MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG

/* Down from the 16 KiB default, which would otherwise dominate the buffer
 * above. IN holds the peer's largest flight - its certificate chain - and OUT
 * this device's own. A device talking to a peer with a longer chain has to
 * raise IN, and the reported buffer margin is where that shows up first. */
#define MBEDTLS_SSL_IN_CONTENT_LEN 4096
#define MBEDTLS_SSL_OUT_CONTENT_LEN 2048

#endif /* APP_CONFIG_MBEDTLS_USER_CONFIG_H */
