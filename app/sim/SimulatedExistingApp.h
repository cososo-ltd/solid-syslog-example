#ifndef APP_SIM_SIMULATED_EXISTING_APP_H
#define APP_SIM_SIMULATED_EXISTING_APP_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /* Bring up the "simulated existing application" — the device SolidSyslog is
     * added to. It:
     *   - brings lwIP UP (LAN9118 netif, static IP) so the networking cost is
     *     the baseline's, never a SolidSyslog delta;
     *   - mounts FatFs (formatting a fresh image on first use) so the filesystem
     *     is likewise baseline;
     *   - opens the mutual-TLS session to the device's broker and holds it, so
     *     what a TLS session costs is likewise baseline (see
     *     SimulatedBrokerSession.h);
     *   - links — but never runs — the rest of the platform surface SolidSyslog
     *     will call from Minimal onward, so its flash is counted below the line.
     *
     * Nothing here logs, or sends anything a collector will see. Runs on a task
     * (netif init, mount and handshake all block). Returns true once all three
     * are up.
     *
     * Must be called after tcpip_init() and after the scheduler has started. */
    bool SimulatedExistingApp_Start(void);

    /* The device's crypto: mbedTLS's allocator, PSA, and the credentials it holds
     * for the mTLS it speaks elsewhere. Separate from the above and callable before
     * the scheduler, because anything that takes an mbedTLS handle has to be given
     * one that was built after the allocator was set — including SolidSyslog, which
     * captures them at create time. */
    bool SimulatedExistingApp_StartCrypto(void);

    /* High-water mark of the static mbedTLS buffer, for sizing it from measurement
     * rather than guesswork. */
    size_t SimulatedExistingApp_MbedTlsPeak(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_SIM_SIMULATED_EXISTING_APP_H */
