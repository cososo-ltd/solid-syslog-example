# solid-syslog-example — run (mtls)

## Device (self-measured)

```text
[device] solid-syslog-example (FreeRTOS + lwIP + mbedTLS + FatFs)
[device] starting simulated existing application...
[sim] broker session to 10.0.2.2:8883: TLSv1.3, TLS1-3-CHACHA20-POLY1305-SHA256
[device]   sim app (lwIP up, FatFs mounted, broker session held over mTLS): ready
[device]   first record logged: yes
[device] provisioning the renewed certificate's pin...
[device] collector renewing its certificate...
[device]   collector renewed: yes
[syslog] WARNING StreamSender category 0x0101 (detail 6)
[syslog] NOTICE StreamSender category 0x0102 (detail 7)
[device] retiring the previous pin...
[device]   records logged before / across / after the renewal: yes
[report] --- SolidSyslog cost above baseline (simulated existing application) ---
[report] key,current,baseline,used_above_baseline
[report] flash_text,368600,352804,15796
[report] flash_data,680,320,360
[report] static_bss,150532,111136,39396
[report] heap_used,4440,4440,0
[report] mbedtls_peak,37320,21360,15960
[report] mbedtls_free,19000,11408,7592
[report] lwip_mem_free,7576,7576,0
[report] lwip_pbufs_free,13,13,0
[report] stack_log,796,116,680
[report] stack_service,3832,56,3776
[report] stack_harness,2872,2840,32
[report] --- end ---
[device]   records logged: 4
[device] ready
```

### Size cross-check

```text
   text	   data	    bss	    dec	    hex	filename
 368592	    688	 150532	 519812	  7ee84	/w/build/baseline-cross/baseline.elf
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
wire   <134>1 2026-09-28T20:33:53.430000Z 10.0.2.15 solid-syslog-example - BOOT [meta sequenceId="1" sysUpTime="243"][timeQuality tzKnown="1" isSynced="0"][origin software="solid-syslog-example" swVersion="0.2.0" enterpriseId="32473" ip="10.0.2.15"][logPipeline@32473 transport="mtls" atRest="hmac-sha256"] ﻿device started
parsed PRIORITY=134 TIMESTAMP=2026-09-28T20:33:53+00:00 HOSTNAME=10.0.2.15 APP_NAME=solid-syslog-example PROCID= MSGID=BOOT STRUCTURED_DATA=[meta sequenceId="1" sysUpTime="243"][timeQuality tzKnown="1" isSynced="0"][origin software="solid-syslog-example" swVersion="0.2.0" enterpriseId="32473" ip="10.0.2.15"][logPipeline@32473 transport="mtls" atRest="hmac-sha256"] MSG=device started
wire   <134>1 2026-09-28T20:33:56.430000Z 10.0.2.15 solid-syslog-example - BOOT [meta sequenceId="2" sysUpTime="543"][timeQuality tzKnown="1" isSynced="0"][origin software="solid-syslog-example" swVersion="0.2.0" enterpriseId="32473" ip="10.0.2.15"][logPipeline@32473 transport="mtls" atRest="hmac-sha256"] ﻿device started
parsed PRIORITY=134 TIMESTAMP=2026-09-28T20:33:56+00:00 HOSTNAME=10.0.2.15 APP_NAME=solid-syslog-example PROCID= MSGID=BOOT STRUCTURED_DATA=[meta sequenceId="2" sysUpTime="543"][timeQuality tzKnown="1" isSynced="0"][origin software="solid-syslog-example" swVersion="0.2.0" enterpriseId="32473" ip="10.0.2.15"][logPipeline@32473 transport="mtls" atRest="hmac-sha256"] MSG=device started
wire   <134>1 2026-09-28T20:33:59.930000Z 10.0.2.15 solid-syslog-example - BOOT [meta sequenceId="3" sysUpTime="893"][timeQuality tzKnown="1" isSynced="0"][origin software="solid-syslog-example" swVersion="0.2.0" enterpriseId="32473" ip="10.0.2.15"][logPipeline@32473 transport="mtls" atRest="hmac-sha256"] ﻿device started
parsed PRIORITY=134 TIMESTAMP=2026-09-28T20:33:59+00:00 HOSTNAME=10.0.2.15 APP_NAME=solid-syslog-example PROCID= MSGID=BOOT STRUCTURED_DATA=[meta sequenceId="3" sysUpTime="893"][timeQuality tzKnown="1" isSynced="0"][origin software="solid-syslog-example" swVersion="0.2.0" enterpriseId="32473" ip="10.0.2.15"][logPipeline@32473 transport="mtls" atRest="hmac-sha256"] MSG=device started
wire   <134>1 2026-09-28T20:34:02.930000Z 10.0.2.15 solid-syslog-example - BOOT [meta sequenceId="4" sysUpTime="1193"][timeQuality tzKnown="1" isSynced="0"][origin software="solid-syslog-example" swVersion="0.2.0" enterpriseId="32473" ip="10.0.2.15"][logPipeline@32473 transport="mtls" atRest="hmac-sha256"] ﻿device started
parsed PRIORITY=134 TIMESTAMP=2026-09-28T20:34:02+00:00 HOSTNAME=10.0.2.15 APP_NAME=solid-syslog-example PROCID= MSGID=BOOT STRUCTURED_DATA=[meta sequenceId="4" sysUpTime="1193"][timeQuality tzKnown="1" isSynced="0"][origin software="solid-syslog-example" swVersion="0.2.0" enterpriseId="32473" ip="10.0.2.15"][logPipeline@32473 transport="mtls" atRest="hmac-sha256"] MSG=device started
```

**RESULT: PASS**
