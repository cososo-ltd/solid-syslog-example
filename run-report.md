# solid-syslog-example — run (buffered)

## Device (self-measured)

```text
[device] solid-syslog-example (FreeRTOS + lwIP + mbedTLS + FatFs)
[device] starting simulated existing application...
[sim] broker session to 10.0.2.2:8883: TLSv1.3, TLS1-3-CHACHA20-POLY1305-SHA256
[device]   sim app (lwIP up, FatFs mounted, broker session held over mTLS): ready
[device]   first record logged: yes
[report] --- SolidSyslog cost above baseline (simulated existing application) ---
[report] key,current,baseline,used_above_baseline
[report] flash_text,356624,349992,6632
[report] flash_data,488,316,172
[report] static_bss,118192,110876,7316
[report] heap_used,4440,4440,0
[report] mbedtls_peak,21320,21332,-12
[report] mbedtls_free,11448,11436,12
[report] lwip_mem_free,7576,7576,0
[report] lwip_pbufs_free,13,14,-1
[report] stack_log,792,120,672
[report] stack_service,1004,52,952
[report] stack_harness,2848,2840,8
[report] --- end ---
[device] ready
```

### Size cross-check

```text
   text	   data	    bss	    dec	    hex	filename
 356616	    496	 118192	 475304	  740a8	/w/build/baseline-cross/baseline.elf
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
wire   <134>1 2026-08-15T14:28:28.430000Z 10.0.2.15 solid-syslog-example - BOOT [meta sequenceId="1"] ﻿device started
parsed PRIORITY=134 TIMESTAMP=2026-08-15T14:28:28+00:00 HOSTNAME=10.0.2.15 APP_NAME=solid-syslog-example PROCID= MSGID=BOOT STRUCTURED_DATA=[meta sequenceId="1"] MSG=device started
```

## Self-check (vs measurements/buffered.csv)

```text
  OK    flash_text: 356624 (expected 356624, Δ0)
  OK    flash_data: 488 (expected 488, Δ0)
  OK    static_bss: 118192 (expected 118192, Δ0)
  OK    heap_used: 4440 (expected 4440, Δ0)
  OK    mbedtls_peak: 21320 (expected 21352, Δ32)
  OK    mbedtls_free: 11448 (expected 11416, Δ32)
  OK    lwip_mem_free: 7576 (expected 7576, Δ0)
  OK    lwip_pbufs_free: 13 (expected 13, Δ0)
  OK    stack_log: 792 (expected 792, Δ0)
  OK    stack_service: 1004 (expected 1004, Δ0)
  OK    stack_harness: 2848 (expected 2848, Δ0)
```

**RESULT: PASS**
