# solid-syslog-example — run (tcp)

## Device (self-measured)

```text
[device] solid-syslog-example (FreeRTOS + lwIP + mbedTLS + FatFs)
[device] starting simulated existing application...
[sim] broker session to 10.0.2.2:8883: TLSv1.3, TLS1-3-CHACHA20-POLY1305-SHA256
[device]   sim app (lwIP up, FatFs mounted, broker session held over mTLS): ready
[device]   first record logged: yes
[report] --- SolidSyslog cost above baseline (simulated existing application) ---
[report] key,current,baseline,used_above_baseline
[report] flash_text,357140,349992,7148
[report] flash_data,488,316,172
[report] static_bss,116560,110876,5684
[report] heap_used,4440,4440,0
[report] mbedtls_peak,21328,21332,-4
[report] mbedtls_free,11440,11436,4
[report] lwip_mem_free,7576,7576,0
[report] lwip_pbufs_free,13,14,-1
[report] stack_log,568,120,448
[report] stack_service,724,52,672
[report] stack_harness,2848,2840,8
[report] --- end ---
[device] ready
```

### Size cross-check

```text
   text	   data	    bss	    dec	    hex	filename
 357132	    496	 116560	 474188	  73c4c	/w/build/baseline-cross/baseline.elf
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
wire   <134>1 2026-07-29T07:30:26.440000Z 10.0.2.15 solid-syslog-example - BOOT [meta sequenceId="1"] ﻿device started
parsed PRIORITY=134 TIMESTAMP=2026-07-29T07:30:26+00:00 HOSTNAME=10.0.2.15 APP_NAME=solid-syslog-example PROCID= MSGID=BOOT STRUCTURED_DATA=[meta sequenceId="1"] MSG=device started
```

## Self-check (vs measurements/tcp.csv)

```text
  OK    flash_text: 357140 (expected 357140, Δ0)
  OK    flash_data: 488 (expected 488, Δ0)
  OK    static_bss: 116560 (expected 116560, Δ0)
  OK    heap_used: 4440 (expected 4440, Δ0)
  OK    mbedtls_peak: 21328 (expected 21328, Δ0)
  OK    mbedtls_free: 11440 (expected 11440, Δ0)
  OK    lwip_mem_free: 7576 (expected 7576, Δ0)
  OK    lwip_pbufs_free: 13 (expected 13, Δ0)
  OK    stack_log: 568 (expected 568, Δ0)
  OK    stack_service: 724 (expected 724, Δ0)
  OK    stack_harness: 2848 (expected 2848, Δ0)
```

**RESULT: PASS**
