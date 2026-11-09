# solid-syslog-example — run (logger)

## Device (self-measured)

```text
[device] solid-syslog-example (FreeRTOS + lwIP + mbedTLS + FatFs)
[syslog] CRITICAL SolidSyslog BAD_CONFIG CREATE_NULL_BUFFER (1)
[syslog] CRITICAL SolidSyslog BAD_CONFIG CREATE_NULL_SENDER (2)
[syslog] CRITICAL SolidSyslog BAD_CONFIG CREATE_NULL_STORE (3)
[device] starting simulated existing application...
[sim] broker session to 10.0.2.2:8883: TLSv1.3, TLS1-3-CHACHA20-POLY1305-SHA256
[device]   sim app (lwIP up, FatFs mounted, broker session held over mTLS): ready
[report] --- SolidSyslog cost above baseline (simulated existing application) ---
[report] key,current,baseline,used_above_baseline
[report] flash_text,355728,352804,2924
[report] flash_data,392,320,72
[report] static_bss,112100,111136,964
[report] heap_used,4440,4440,0
[report] mbedtls_peak,21360,21360,0
[report] mbedtls_free,11408,11408,0
[report] lwip_mem_free,7576,7576,0
[report] lwip_pbufs_free,13,13,0
[report] stack_log,116,116,0
[report] stack_service,56,56,0
[report] stack_harness,2840,2840,0
[report] --- end ---
[device]   records logged: 0
[device] ready
```

### Size cross-check

```text
   text	   data	    bss	    dec	    hex	filename
 355720	    400	 112100	 468220	  724fc	/w/build/baseline-cross/baseline.elf
```

## Listeners (proved before the device ran)

```text
  OK    udp    5514
  OK    tcp    5601
  OK    tls    6514
  OK    mtls   6515
  OK    mtls   6515 — refused a client with no certificate
  OK    broker 8883
  OK    broker 8883 — refused a client with no certificate
```

## Collector (syslog-ng) received

```text
(nothing — this device sends no records yet)
```

**RESULT: PASS**
