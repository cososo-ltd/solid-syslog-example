# solid-syslog-example — run (file-store)

## Device (self-measured)

```text
[device] solid-syslog-example (FreeRTOS + lwIP + mbedTLS + FatFs)
[device] starting simulated existing application...
[sim] broker session to 10.0.2.2:8883: TLSv1.3, TLS1-3-CHACHA20-POLY1305-SHA256
[device]   sim app (lwIP up, FatFs mounted, broker session held over mTLS): ready
[device]   first record logged: yes
[report] --- SolidSyslog cost above baseline (simulated existing application) ---
[report] key,current,baseline,used_above_baseline
[report] flash_text,375056,356996,18060
[report] flash_data,564,236,328
[report] static_bss,121244,111452,9792
[report] heap_used,4440,4440,0
[report] mbedtls_peak,21008,20996,12
[report] mbedtls_free,11760,11772,-12
[report] lwip_mem_free,7576,7576,0
[report] lwip_pbufs_free,13,13,0
[report] stack_log,828,116,712
[report] stack_service,520,56,464
[report] stack_harness,3040,3040,0
[report] --- end ---
[device]   records logged: 1
[device] ready
```

### Size cross-check

```text
   text	   data	    bss	    dec	    hex	filename
 375048	    572	 121244	 496864	  794e0	/w/build/baseline-cross/baseline.elf
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
wire   <134>1 2026-11-27T22:56:49.330000Z 10.0.2.15 solid-syslog-example - BOOT [meta sequenceId="1" sysUpTime="233"][timeQuality tzKnown="1" isSynced="0"] ﻿device started
parsed PRIORITY=134 TIMESTAMP=2026-11-27T22:56:49+00:00 HOSTNAME=10.0.2.15 APP_NAME=solid-syslog-example PROCID= MSGID=BOOT STRUCTURED_DATA=[meta sequenceId="1" sysUpTime="233"][timeQuality tzKnown="1" isSynced="0"] MSG=device started
```

**RESULT: PASS**
