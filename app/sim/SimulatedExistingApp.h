#ifndef APP_SIM_SIMULATED_EXISTING_APP_H
#define APP_SIM_SIMULATED_EXISTING_APP_H

#include <stdbool.h>

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
     *   - links — but never runs — the mbedTLS client surface plus the lwIP raw
     *     UDP/TCP and FatFs file APIs SolidSyslog will call from Minimal onward, so their
     *     flash is counted below the line.
     *
     * Nothing here emits, sends, stores, or handshakes. Runs on a task (netif
     * init and mount both block). Returns true once lwIP is up and FatFs mounted.
     *
     * Must be called after tcpip_init() and after the scheduler has started. */
    bool SimulatedExistingApp_Start(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_SIM_SIMULATED_EXISTING_APP_H */
