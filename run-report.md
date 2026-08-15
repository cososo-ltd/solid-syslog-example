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
[report] flash_text,357156,349992,7164
[report] flash_data,488,316,172
[report] static_bss,118372,110876,7496
[report] heap_used,4440,4440,0
[report] mbedtls_peak,21320,21332,-12
[report] mbedtls_free,11448,11436,12
[report] lwip_mem_free,7576,7576,0
[report] lwip_pbufs_free,13,14,-1
[report] stack_log,792,120,672
[report] stack_service,948,52,896
[report] stack_harness,2848,2840,8
[report] --- end ---
[device] ready
```

### Size cross-check

```text
   text	   data	    bss	    dec	    hex	filename
 357148	    496	 118372	 476016	  74370	/w/build/baseline-cross/baseline.elf
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
wire   <134>1 2026-08-15T21:46:49.380000Z 10.0.2.15 solid-syslog-example - BOOT [meta sequenceId="1"] ﻿device started
parsed PRIORITY=134 TIMESTAMP=2026-08-15T21:46:49+00:00 HOSTNAME=10.0.2.15 APP_NAME=solid-syslog-example PROCID= MSGID=BOOT STRUCTURED_DATA=[meta sequenceId="1"] MSG=device started
```

## Self-check (vs measurements/tcp.csv)

```text
  OK    flash_text: 357156 (expected 357156, Δ0)
  OK    flash_data: 488 (expected 488, Δ0)
  OK    static_bss: 118372 (expected 118372, Δ0)
  OK    heap_used: 4440 (expected 4440, Δ0)
  OK    mbedtls_peak: 21320 (expected 21316, Δ4)
  OK    mbedtls_free: 11448 (expected 11452, Δ4)
  OK    lwip_mem_free: 7576 (expected 7576, Δ0)
  OK    lwip_pbufs_free: 13 (expected 14, Δ1)
  OK    stack_log: 792 (expected 792, Δ0)
  OK    stack_service: 948 (expected 948, Δ0)
  OK    stack_harness: 2848 (expected 2848, Δ0)
```

**RESULT: PASS**
