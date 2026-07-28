/* The mutual-TLS session this device already holds to its broker — over lwIP's
 * raw TCP and mbedTLS, on the credentials DeviceCertStore hands out.
 *
 * It exists so the measured mbedTLS cost is a session's cost, not a linked
 * library's: anything added on top is then charged for a *second* concurrent
 * session, which is far less. Held open for the life of the device, because two
 * sessions that never overlap would measure max() rather than sum().
 *
 * On a real device the peer is an MQTT broker or a cloud gateway carrying
 * application traffic. Here it is an openssl s_server and one exchange — the
 * session is what is being measured, not the protocol. */
#ifndef APP_SIM_SIMULATED_BROKER_SESSION_H
#define APP_SIM_SIMULATED_BROKER_SESSION_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /** Connect, authenticate both ways, and leave the session open. False if the
     *  broker is unreachable, the handshake fails, or the peer does not verify —
     *  on a device that is a failed bring-up, and here it fails the run, because
     *  a session that did not open is memory the baseline did not spend. */
    bool SimulatedBrokerSession_Open(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_SIM_SIMULATED_BROKER_SESSION_H */
