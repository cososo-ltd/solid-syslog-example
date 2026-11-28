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
[syslog] WARNING StreamSender SENDER_DELIVERY_FAILED DELIVERY_FAILED (6)
[syslog] NOTICE StreamSender SENDER_DELIVERY_RESTORED DELIVERY_RESTORED (7)
[device] retiring the previous pin...
[device]   records logged before / across / after the renewal: yes
[report] --- SolidSyslog cost above baseline (simulated existing application) ---
[report] key,current,baseline,used_above_baseline
[report] flash_text,380184,356996,23188
[report] flash_data,604,236,368
[report] static_bss,151816,111452,40364
[report] heap_used,4440,4440,0
[report] mbedtls_peak,36992,20996,15996
[report] mbedtls_free,19328,11772,7556
[report] lwip_mem_free,7576,7576,0
[report] lwip_pbufs_free,13,13,0
[report] stack_log,828,116,712
[report] stack_service,3592,56,3536
[report] stack_harness,3072,3040,32
[report] --- end ---
[device]   records logged: 4
[device] ready
```

### Size cross-check

```text
   text	   data	    bss	    dec	    hex	filename
 380176	    612	 151816	 532604	  8207c	/w/build/baseline-cross/baseline.elf
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
wire   <134>1 2026-11-27T23:03:45.330000Z 10.0.2.15 solid-syslog-example - BOOT [meta sequenceId="1" sysUpTime="233"][timeQuality tzKnown="1" isSynced="0"][origin software="solid-syslog-example" swVersion="0.4.0" enterpriseId="32473" ip="10.0.2.15"][logPipeline@32473 transport="mtls" atRest="hmac-sha256"] ﻿device started
parsed PRIORITY=134 TIMESTAMP=2026-11-27T23:03:45+00:00 HOSTNAME=10.0.2.15 APP_NAME=solid-syslog-example PROCID= MSGID=BOOT STRUCTURED_DATA=[meta sequenceId="1" sysUpTime="233"][timeQuality tzKnown="1" isSynced="0"][origin software="solid-syslog-example" swVersion="0.4.0" enterpriseId="32473" ip="10.0.2.15"][logPipeline@32473 transport="mtls" atRest="hmac-sha256"] MSG=device started
wire   <134>1 2026-11-27T23:03:48.330000Z 10.0.2.15 solid-syslog-example - BOOT [meta sequenceId="2" sysUpTime="533"][timeQuality tzKnown="1" isSynced="0"][origin software="solid-syslog-example" swVersion="0.4.0" enterpriseId="32473" ip="10.0.2.15"][logPipeline@32473 transport="mtls" atRest="hmac-sha256"] ﻿device started
parsed PRIORITY=134 TIMESTAMP=2026-11-27T23:03:48+00:00 HOSTNAME=10.0.2.15 APP_NAME=solid-syslog-example PROCID= MSGID=BOOT STRUCTURED_DATA=[meta sequenceId="2" sysUpTime="533"][timeQuality tzKnown="1" isSynced="0"][origin software="solid-syslog-example" swVersion="0.4.0" enterpriseId="32473" ip="10.0.2.15"][logPipeline@32473 transport="mtls" atRest="hmac-sha256"] MSG=device started
wire   <134>1 2026-11-27T23:03:51.830000Z 10.0.2.15 solid-syslog-example - BOOT [meta sequenceId="3" sysUpTime="883"][timeQuality tzKnown="1" isSynced="0"][origin software="solid-syslog-example" swVersion="0.4.0" enterpriseId="32473" ip="10.0.2.15"][logPipeline@32473 transport="mtls" atRest="hmac-sha256"] ﻿device started
parsed PRIORITY=134 TIMESTAMP=2026-11-27T23:03:51+00:00 HOSTNAME=10.0.2.15 APP_NAME=solid-syslog-example PROCID= MSGID=BOOT STRUCTURED_DATA=[meta sequenceId="3" sysUpTime="883"][timeQuality tzKnown="1" isSynced="0"][origin software="solid-syslog-example" swVersion="0.4.0" enterpriseId="32473" ip="10.0.2.15"][logPipeline@32473 transport="mtls" atRest="hmac-sha256"] MSG=device started
wire   <44>1 2026-11-27T23:03:51.860000Z 10.0.2.15 solid-syslog-example - SENDER_DELIVERY_FAILED [meta sequenceId="4" sysUpTime="886"][timeQuality tzKnown="1" isSynced="0"][origin software="solid-syslog-example" swVersion="0.4.0" enterpriseId="32473" ip="10.0.2.15"][logPipeline@32473 transport="mtls" atRest="hmac-sha256"][solidsyslog@67478 source="StreamSender" category="SENDER_DELIVERY_FAILED" detail="DELIVERY_FAILED"] ﻿SolidSyslog event
parsed PRIORITY=44 TIMESTAMP=2026-11-27T23:03:51+00:00 HOSTNAME=10.0.2.15 APP_NAME=solid-syslog-example PROCID= MSGID=SENDER_DELIVERY_FAILED STRUCTURED_DATA=[meta sequenceId="4" sysUpTime="886"][timeQuality tzKnown="1" isSynced="0"][origin software="solid-syslog-example" swVersion="0.4.0" enterpriseId="32473" ip="10.0.2.15"][logPipeline@32473 transport="mtls" atRest="hmac-sha256"][solidsyslog@67478 source="StreamSender" category="SENDER_DELIVERY_FAILED" detail="DELIVERY_FAILED"] MSG=SolidSyslog event
wire   <45>1 2026-11-27T23:03:52.090000Z 10.0.2.15 solid-syslog-example - SENDER_DELIVERY_RESTORED [meta sequenceId="5" sysUpTime="909"][timeQuality tzKnown="1" isSynced="0"][origin software="solid-syslog-example" swVersion="0.4.0" enterpriseId="32473" ip="10.0.2.15"][logPipeline@32473 transport="mtls" atRest="hmac-sha256"][solidsyslog@67478 source="StreamSender" category="SENDER_DELIVERY_RESTORED" detail="DELIVERY_RESTORED"] ﻿SolidSyslog event
parsed PRIORITY=45 TIMESTAMP=2026-11-27T23:03:52+00:00 HOSTNAME=10.0.2.15 APP_NAME=solid-syslog-example PROCID= MSGID=SENDER_DELIVERY_RESTORED STRUCTURED_DATA=[meta sequenceId="5" sysUpTime="909"][timeQuality tzKnown="1" isSynced="0"][origin software="solid-syslog-example" swVersion="0.4.0" enterpriseId="32473" ip="10.0.2.15"][logPipeline@32473 transport="mtls" atRest="hmac-sha256"][solidsyslog@67478 source="StreamSender" category="SENDER_DELIVERY_RESTORED" detail="DELIVERY_RESTORED"] MSG=SolidSyslog event
wire   <134>1 2026-11-27T23:03:54.830000Z 10.0.2.15 solid-syslog-example - BOOT [meta sequenceId="6" sysUpTime="1183"][timeQuality tzKnown="1" isSynced="0"][origin software="solid-syslog-example" swVersion="0.4.0" enterpriseId="32473" ip="10.0.2.15"][logPipeline@32473 transport="mtls" atRest="hmac-sha256"] ﻿device started
parsed PRIORITY=134 TIMESTAMP=2026-11-27T23:03:54+00:00 HOSTNAME=10.0.2.15 APP_NAME=solid-syslog-example PROCID= MSGID=BOOT STRUCTURED_DATA=[meta sequenceId="6" sysUpTime="1183"][timeQuality tzKnown="1" isSynced="0"][origin software="solid-syslog-example" swVersion="0.4.0" enterpriseId="32473" ip="10.0.2.15"][logPipeline@32473 transport="mtls" atRest="hmac-sha256"] MSG=device started
```

**RESULT: PASS**
