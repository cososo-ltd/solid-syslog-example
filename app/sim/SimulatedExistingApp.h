#ifndef APP_SIM_SIMULATED_EXISTING_APP_H
#define APP_SIM_SIMULATED_EXISTING_APP_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /* Bring the netif up, mount the volume, and open the broker session. Blocks
     * on all three, so it needs a task, and it needs tcpip_init() done first. */
    bool SimulatedExistingApp_Start(void);

    /* mbedTLS's allocator, PSA, and the credentials. Separate because it must run
     * before the scheduler: an mbedTLS handle is only usable if it was built
     * after the allocator was set, and callers capture handles at create time. */
    bool SimulatedExistingApp_StartCrypto(void);

    /* High-water mark of the static mbedTLS buffer, and what was left of it
     * there — the two figures the buffer is sized from. */
    size_t SimulatedExistingApp_MbedTlsPeak(void);
    size_t SimulatedExistingApp_MbedTlsFree(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_SIM_SIMULATED_EXISTING_APP_H */
