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
[report] flash_text,356608,349992,6616
[report] flash_data,488,316,172
[report] static_bss,116380,110876,5504
[report] heap_used,4440,4440,0
[report] mbedtls_peak,21200,21332,-132
[report] mbedtls_free,11568,11436,132
[report] lwip_mem_free,7576,7576,0
[report] lwip_pbufs_free,13,14,-1
[report] stack_log,568,120,448
[report] stack_service,780,52,728
[report] stack_harness,2848,2840,8
[report] --- end ---
[device] ready
```

### Size cross-check

```text
   text	   data	    bss	    dec	    hex	filename
 356600	    496	 116380	 473476	  73984	/w/build/baseline-cross/baseline.elf
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
wire   <134>1 2026-07-29T07:17:47.410000Z 10.0.2.15 solid-syslog-example - BOOT [meta sequenceId="1"] ﻿device started
parsed PRIORITY=134 TIMESTAMP=2026-07-29T07:17:47+00:00 HOSTNAME=10.0.2.15 APP_NAME=solid-syslog-example PROCID= MSGID=BOOT STRUCTURED_DATA=[meta sequenceId="1"] MSG=device started
```

## Self-check (vs measurements/buffered.csv)

```text
  OK    flash_text: 356608 (expected 356608, Δ0)
  OK    flash_data: 488 (expected 488, Δ0)
  OK    static_bss: 116380 (expected 116380, Δ0)
  OK    heap_used: 4440 (expected 4440, Δ0)
  OK    mbedtls_peak: 21200 (expected 21316, Δ116)
  OK    mbedtls_free: 11568 (expected 11452, Δ116)
  OK    lwip_mem_free: 7576 (expected 7576, Δ0)
  OK    lwip_pbufs_free: 13 (expected 13, Δ0)
  OK    stack_log: 568 (expected 568, Δ0)
  OK    stack_service: 780 (expected 780, Δ0)
  OK    stack_harness: 2848 (expected 2848, Δ0)
```

**RESULT: PASS**
