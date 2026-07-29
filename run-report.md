# solid-syslog-example — run (origin)

## Device (self-measured)

```text
[device] solid-syslog-example (FreeRTOS + lwIP + mbedTLS + FatFs)
[device] starting simulated existing application...
[sim] broker session to 10.0.2.2:8883: TLSv1.3, TLS1-3-CHACHA20-POLY1305-SHA256
[device]   sim app (lwIP up, FatFs mounted, broker session held over mTLS): ready
[device]   first record logged: yes
[report] --- SolidSyslog cost above baseline (simulated existing application) ---
[report] key,current,baseline,used_above_baseline
[report] flash_text,361640,349992,11648
[report] flash_data,640,316,324
[report] static_bss,117688,110876,6812
[report] heap_used,4440,4440,0
[report] mbedtls_peak,21336,21332,4
[report] mbedtls_free,11432,11436,-4
[report] lwip_mem_free,7576,7576,0
[report] lwip_pbufs_free,13,14,-1
[report] stack_log,568,120,448
[report] stack_service,788,52,736
[report] stack_harness,2848,2840,8
[report] --- end ---
[device] ready
```

### Size cross-check

```text
   text	   data	    bss	    dec	    hex	filename
 361632	    648	 117688	 479968	  752e0	/w/build/baseline-cross/baseline.elf
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
wire   <134>1 2026-07-29T08:21:38.360000Z 10.0.2.15 solid-syslog-example - BOOT [meta sequenceId="1" sysUpTime="236"][timeQuality tzKnown="1" isSynced="0"][origin software="solid-syslog-example" swVersion="0.1.0" enterpriseId="32473"] ﻿device started
parsed PRIORITY=134 TIMESTAMP=2026-07-29T08:21:38+00:00 HOSTNAME=10.0.2.15 APP_NAME=solid-syslog-example PROCID= MSGID=BOOT STRUCTURED_DATA=[meta sequenceId="1" sysUpTime="236"][timeQuality tzKnown="1" isSynced="0"][origin software="solid-syslog-example" swVersion="0.1.0" enterpriseId="32473"] MSG=device started
```

## Self-check (vs measurements/origin.csv)

```text
  OK    flash_text: 361640 (expected 361640, Δ0)
  OK    flash_data: 640 (expected 640, Δ0)
  OK    static_bss: 117688 (expected 117688, Δ0)
  OK    heap_used: 4440 (expected 4440, Δ0)
  OK    mbedtls_peak: 21336 (expected 21336, Δ0)
  OK    mbedtls_free: 11432 (expected 11432, Δ0)
  OK    lwip_mem_free: 7576 (expected 7576, Δ0)
  OK    lwip_pbufs_free: 13 (expected 13, Δ0)
  OK    stack_log: 568 (expected 568, Δ0)
  OK    stack_service: 788 (expected 788, Δ0)
  OK    stack_harness: 2848 (expected 2848, Δ0)
```

**RESULT: PASS**
