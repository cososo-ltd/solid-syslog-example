# solid-syslog-example — run (header-fields)

## Device (self-measured)

```text
[device] solid-syslog-example (FreeRTOS + lwIP + mbedTLS + FatFs)
[device] starting simulated existing application...
[sim] broker session to 10.0.2.2:8883: TLSv1.3, TLS1-3-CHACHA20-POLY1305-SHA256
[device]   sim app (lwIP up, FatFs mounted, broker session held over mTLS): ready
[device]   first record logged: yes
[report] --- SolidSyslog cost above baseline (simulated existing application) ---
[report] key,current,baseline,used_above_baseline
[report] flash_text,354960,349992,4968
[report] flash_data,448,316,132
[report] static_bss,111116,110876,240
[report] heap_used,4440,4440,0
[report] mbedtls_peak,21284,21332,-48
[report] mbedtls_free,11484,11436,48
[report] lwip_mem_free,7576,7576,0
[report] lwip_pbufs_free,13,14,-1
[report] stack_log,136,120,16
[report] stack_service,52,52,0
[report] stack_harness,2848,2840,8
[report] --- end ---
[device] ready
```

### Size cross-check

```text
   text	   data	    bss	    dec	    hex	filename
 354952	    456	 111116	 466524	  71e5c	/w/build/baseline-cross/baseline.elf
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
wire   <134>1 2026-07-29T07:14:15.310000Z 10.0.2.15 solid-syslog-example - BOOT - ﻿device started
parsed PRIORITY=134 TIMESTAMP=2026-07-29T07:14:15+00:00 HOSTNAME=10.0.2.15 APP_NAME=solid-syslog-example PROCID= MSGID=BOOT STRUCTURED_DATA= MSG=device started
```

## Self-check (vs measurements/header-fields.csv)

```text
  OK    flash_text: 354960 (expected 354960, Δ0)
  OK    flash_data: 448 (expected 448, Δ0)
  OK    static_bss: 111116 (expected 111116, Δ0)
  OK    heap_used: 4440 (expected 4440, Δ0)
  OK    mbedtls_peak: 21284 (expected 21216, Δ68)
  OK    mbedtls_free: 11484 (expected 11552, Δ68)
  OK    lwip_mem_free: 7576 (expected 7576, Δ0)
  OK    lwip_pbufs_free: 13 (expected 14, Δ1)
  OK    stack_log: 136 (expected 136, Δ0)
  OK    stack_service: 52 (expected 52, Δ0)
  OK    stack_harness: 2848 (expected 2848, Δ0)
```

**RESULT: PASS**
