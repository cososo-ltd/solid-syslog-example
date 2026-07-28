/* The site's log collector as the device sees it when the collector's certificate
 * is renewed: the operator hands over the renewed certificate's pin ahead of time,
 * and the collector later restarts with that certificate.
 *
 * Harness only. A real device receives a new pin over its management channel and
 * never asks its collector to renew. */
#ifndef APP_SIM_SIMULATED_COLLECTOR_H
#define APP_SIM_SIMULATED_COLLECTOR_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /** The fingerprint of the certificate the collector will renew to, in the
     *  RFC 5425 section 4.2.2 form. NULL if it cannot be read. */
    const char* SimulatedCollector_NextPin(void);

    /** Restart the collector with its renewed certificate, which drops every open
     *  session, and wait until it is listening again. False if it did not
     *  confirm within `timeoutSeconds` of host time. */
    bool SimulatedCollector_Renew(uint32_t timeoutSeconds);

#ifdef __cplusplus
}
#endif

#endif /* APP_SIM_SIMULATED_COLLECTOR_H */
