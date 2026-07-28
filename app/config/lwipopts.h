/* lwIP options for this device on QEMU mps2-an385 (FreeRTOS + lwIP, NO_SYS=0).
 *
 * lwIP runs its own "tcpip" thread and a LAN9118 netif (net/EthernetIf.c) is
 * brought up, so the netif and EMAC cost is the device's. Only the Raw API is
 * used — the sequential netconn / socket API stays OFF; the tcpip thread exists
 * for RX delivery (tcpip_input), timeouts, and marshalled callbacks.
 *
 * The pools below are the largest single block of this device's static RAM. Each
 * is either lwIP's own default or sized against another setting here, never
 * against what one run happened to use — and the run reports how much of each
 * was actually reached. */
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
/* This device reaches its peers by address, so it carries no resolver. */
#define LWIP_DNS 0

/* etharp queues the first packet to a destination while ARP resolves it —
 * keep queueing on so a first datagram after boot is not dropped. */
#define ARP_QUEUEING 1

/* --- Memory: lwIP-managed static pools (no libc/posix heap) ----------- */
#define MEM_LIBC_MALLOC 0
#define MEMP_MEM_MALLOC 0
#define MEM_ALIGNMENT 4
/* Sized against TCP_SND_BUF below, not against what this device measures using:
 * tcp_write copies into PBUF_RAM out of here, so anything less than the send
 * buffer is a window lwIP could never fill. The device's own traffic peaks far
 * below it, which the run reports rather than assumes. */
#define MEM_SIZE (8 * 1024)

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
/* Memory statistics, and kept on rather than used once and removed: the pools
 * above are the largest single block of this device's static RAM, and their
 * high-water marks are the only thing that says whether those numbers are sized
 * or merely inherited. The protocol counters stay off — they cost code and
 * answer nothing this device asks. */
#define LWIP_STATS 1
#define LWIP_STATS_DISPLAY 0
#define MEM_STATS 1
#define MEMP_STATS 1
#define LINK_STATS 0
#define ETHARP_STATS 0
#define IP_STATS 0
#define IPFRAG_STATS 0
#define ICMP_STATS 0
#define UDP_STATS 0
#define TCP_STATS 0
#define SYS_STATS 0
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
