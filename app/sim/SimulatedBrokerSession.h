/* The mutual-TLS session this device already holds to its cloud broker.
 *
 * A secured device does not link a TLS stack and leave it idle — it keeps a
 * session open to something. This is that session: opened at bring-up and held
 * for the life of the device, over lwIP's raw TCP API and mbedTLS, using the
 * same provisioned credentials DeviceCertStore hands out.
 *
 * It exists so the baseline's mbedTLS cost is a session's cost. What SolidSyslog
 * then adds is what a *second* concurrent session adds, which is measured rather
 * than asserted. Whether a given product's two connections truly overlap is the
 * product's business; this one assumes they do, which is the conservative
 * reading for the baseline and the honest one for us.
 *
 * On a real device the peer is an MQTT broker, a device-management service or a
 * cloud gateway, and the session carries application traffic. Here the far end
 * is an openssl s_server (see docker/docker-compose.yml) and the traffic is one
 * exchange, because what is being measured is the session, not the protocol. */
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
