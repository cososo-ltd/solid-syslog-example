/* lwIP options for the Baseline device on QEMU mps2-an385 (FreeRTOS + lwIP,
 * NO_SYS=0). Adapted from solid-syslog Bdd/Targets/FreeRtosLwip/lwipopts.h: the
 * OS-abstraction, protocol, memory-pool, and TCP-sizing knobs are kept as-is so
 * the baseline ships the same static, embedded-realistic lwIP footprint an
 * integrator would; the only change is DNS turned off (the baseline resolves no
 * names and opens no session to a collector).
 *
 * lwIP runs its own "tcpip" thread; a LAN9118 netif (net/EthernetIf.c) is
 * brought up so the netif/EMAC cost is in the baseline. Only the Raw API is
 * used — the sequential netconn / socket API stays OFF; the tcpip thread exists
 * for RX delivery (tcpip_input), timeouts, and marshalled callbacks. */
#ifndef APP_CONFIG_LWIPOPTS_H
#define APP_CONFIG_LWIPOPTS_H

/* --- OS abstraction (NO_SYS=0: tcpip thread + FreeRTOS sys_arch) ------- */
#define NO_SYS 0
#define SYS_LIGHTWEIGHT_PROT 1
#define LWIP_TCPIP_CORE_LOCKING 1
/* Raw API only — no sequential netconn / BSD-socket API. */
#define LWIP_NETCONN 0
#define LWIP_SOCKET 0

/* tcpip thread. Priorities are numeric here: lwipopts.h is processed via
 * lwip/opt.h before any FreeRTOS header, so configMAX_PRIORITIES (7) is not
 * visible — TCPIP_THREAD_PRIO 6 == configMAX_PRIORITIES - 1, above the
 * LAN9118 RX task (configMAX_PRIORITIES - 2 == 5, set in EthernetIf.c).
 * TCPIP_THREAD_STACKSIZE is in BYTES (LWIP_FREERTOS_THREAD_STACKSIZE_IS_
 * STACKWORDS defaults to 0, so the sys_arch divides by sizeof(StackType_t)). */
/* lwIP's api/err.c maps err_t to errno; pull the E* codes from newlib's
 * <errno.h> rather than have lwIP provide its own (which would clash with
 * newlib's definitions). */
#define LWIP_ERRNO_STDINCLUDE 1

#define TCPIP_THREAD_NAME "tcpip"
#define TCPIP_THREAD_STACKSIZE 4096
#define TCPIP_THREAD_PRIO 6
#define TCPIP_MBOX_SIZE 8
#define DEFAULT_RAW_RECVMBOX_SIZE 8
#define DEFAULT_UDP_RECVMBOX_SIZE 8
#define DEFAULT_TCP_RECVMBOX_SIZE 8
#define DEFAULT_ACCEPTMBOX_SIZE 8

/* --- Protocol surface ------------------------------------------------- */
#define LWIP_IPV4 1
#define LWIP_IPV6 0
#define LWIP_RAW 1
#define LWIP_UDP 1
#define LWIP_TCP 1
#define LWIP_ARP 1
#define LWIP_DHCP 0
#define LWIP_ICMP 1
#define LWIP_IGMP 0

/* --- DNS: off --------------------------------------------------------- */
/* The baseline brings lwIP up but resolves nothing by name — it opens no
 * outbound session to any collector. SolidSyslog introduces collector name
 * resolution at Secure; the baseline does not carry it. */
#define LWIP_DNS 0

/* etharp queues the first packet to a destination while ARP resolves it —
 * keep queueing on so a first datagram after boot is not dropped. */
#define ARP_QUEUEING 1

/* --- Memory: lwIP-managed static pools (no libc/posix heap) ----------- */
#define MEM_LIBC_MALLOC 0
#define MEMP_MEM_MALLOC 0
#define MEM_ALIGNMENT 4
#define MEM_SIZE (16 * 1024)

#define MEMP_NUM_UDP_PCB 4
#define MEMP_NUM_TCP_PCB 4
#define MEMP_NUM_TCP_PCB_LISTEN 2
#define MEMP_NUM_TCP_SEG 16
#define MEMP_NUM_PBUF 16
#define MEMP_NUM_RAW_PCB 2
#define MEMP_NUM_ARP_QUEUE 4
/* tcpip thread message pools: API callbacks (the marshal) + inbound packets
 * posted by the netif RX task via tcpip_input. */
#define MEMP_NUM_TCPIP_MSG_API 8
#define MEMP_NUM_TCPIP_MSG_INPKT 8

#define PBUF_POOL_SIZE 16

/* --- TCP sizing ------------------------------------------------------- */
#define TCP_MSS 1460
#define TCP_WND (4 * TCP_MSS)
#define TCP_SND_BUF (4 * TCP_MSS)
#define TCP_QUEUE_OOSEQ 0

/* --- Diagnostics ------------------------------------------------------ */
#define LWIP_STATS 0
#define LWIP_NETIF_API 0
#define LWIP_NETIF_STATUS_CALLBACK 0
#define LWIP_NETIF_LINK_CALLBACK 0
#define CHECKSUM_GEN_IP 1
#define CHECKSUM_GEN_UDP 1
#define CHECKSUM_GEN_TCP 1
#define CHECKSUM_CHECK_IP 1
#define CHECKSUM_CHECK_UDP 1
#define CHECKSUM_CHECK_TCP 1

#endif /* APP_CONFIG_LWIPOPTS_H */
