# solid-syslog-example — run (chain)

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
[report] flash_text,376220,352804,23416
[report] flash_data,692,320,372
[report] static_bss,151532,111136,40396
[report] heap_used,4440,4440,0
[report] mbedtls_peak,37316,21360,15956
[report] mbedtls_free,19004,11408,7596
[report] lwip_mem_free,7576,7576,0
[report] lwip_pbufs_free,13,13,0
[report] stack_log,828,116,712
[report] stack_service,3400,56,3344
[report] stack_harness,2872,2840,32
[report] --- end ---
[device]   records logged: 4
[device] ready
```

### Size cross-check

```text
   text	   data	    bss	    dec	    hex	filename
 376212	    700	 151532	 528444	  8103c	/w/build/baseline-cross/baseline.elf
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
wire   <134>1 2026-11-08T15:48:49.420000Z 10.0.2.15 solid-syslog-example - BOOT [meta sequenceId="1" sysUpTime="242"][timeQuality tzKnown="1" isSynced="0"][origin software="solid-syslog-example" swVersion="0.4.0" enterpriseId="32473" ip="10.0.2.15"][logPipeline@32473 transport="mtls" collectorAuth="fingerprint+chain" atRest="aes-256-gcm"] ﻿device started
parsed PRIORITY=134 TIMESTAMP=2026-11-08T15:48:49+00:00 HOSTNAME=10.0.2.15 APP_NAME=solid-syslog-example PROCID= MSGID=BOOT STRUCTURED_DATA=[meta sequenceId="1" sysUpTime="242"][timeQuality tzKnown="1" isSynced="0"][origin software="solid-syslog-example" swVersion="0.4.0" enterpriseId="32473" ip="10.0.2.15"][logPipeline@32473 transport="mtls" collectorAuth="fingerprint+chain" atRest="aes-256-gcm"] MSG=device started
wire   <134>1 2026-11-08T15:48:52.420000Z 10.0.2.15 solid-syslog-example - BOOT [meta sequenceId="2" sysUpTime="542"][timeQuality tzKnown="1" isSynced="0"][origin software="solid-syslog-example" swVersion="0.4.0" enterpriseId="32473" ip="10.0.2.15"][logPipeline@32473 transport="mtls" collectorAuth="fingerprint+chain" atRest="aes-256-gcm"] ﻿device started
parsed PRIORITY=134 TIMESTAMP=2026-11-08T15:48:52+00:00 HOSTNAME=10.0.2.15 APP_NAME=solid-syslog-example PROCID= MSGID=BOOT STRUCTURED_DATA=[meta sequenceId="2" sysUpTime="542"][timeQuality tzKnown="1" isSynced="0"][origin software="solid-syslog-example" swVersion="0.4.0" enterpriseId="32473" ip="10.0.2.15"][logPipeline@32473 transport="mtls" collectorAuth="fingerprint+chain" atRest="aes-256-gcm"] MSG=device started
wire   <134>1 2026-11-08T15:48:55.820000Z 10.0.2.15 solid-syslog-example - BOOT [meta sequenceId="3" sysUpTime="882"][timeQuality tzKnown="1" isSynced="0"][origin software="solid-syslog-example" swVersion="0.4.0" enterpriseId="32473" ip="10.0.2.15"][logPipeline@32473 transport="mtls" collectorAuth="fingerprint+chain" atRest="aes-256-gcm"] ﻿device started
parsed PRIORITY=134 TIMESTAMP=2026-11-08T15:48:55+00:00 HOSTNAME=10.0.2.15 APP_NAME=solid-syslog-example PROCID= MSGID=BOOT STRUCTURED_DATA=[meta sequenceId="3" sysUpTime="882"][timeQuality tzKnown="1" isSynced="0"][origin software="solid-syslog-example" swVersion="0.4.0" enterpriseId="32473" ip="10.0.2.15"][logPipeline@32473 transport="mtls" collectorAuth="fingerprint+chain" atRest="aes-256-gcm"] MSG=device started
wire   <44>1 2026-11-08T15:48:55.850000Z 10.0.2.15 solid-syslog-example - SENDER_DELIVERY_FAILED [meta sequenceId="4" sysUpTime="885"][timeQuality tzKnown="1" isSynced="0"][origin software="solid-syslog-example" swVersion="0.4.0" enterpriseId="32473" ip="10.0.2.15"][logPipeline@32473 transport="mtls" collectorAuth="fingerprint+chain" atRest="aes-256-gcm"][solidsyslog@67478 source="StreamSender" category="SENDER_DELIVERY_FAILED" detail="DELIVERY_FAILED"] ﻿SolidSyslog event
parsed PRIORITY=44 TIMESTAMP=2026-11-08T15:48:55+00:00 HOSTNAME=10.0.2.15 APP_NAME=solid-syslog-example PROCID= MSGID=SENDER_DELIVERY_FAILED STRUCTURED_DATA=[meta sequenceId="4" sysUpTime="885"][timeQuality tzKnown="1" isSynced="0"][origin software="solid-syslog-example" swVersion="0.4.0" enterpriseId="32473" ip="10.0.2.15"][logPipeline@32473 transport="mtls" collectorAuth="fingerprint+chain" atRest="aes-256-gcm"][solidsyslog@67478 source="StreamSender" category="SENDER_DELIVERY_FAILED" detail="DELIVERY_FAILED"] MSG=SolidSyslog event
wire   <45>1 2026-11-08T15:48:56.220000Z 10.0.2.15 solid-syslog-example - SENDER_DELIVERY_RESTORED [meta sequenceId="5" sysUpTime="922"][timeQuality tzKnown="1" isSynced="0"][origin software="solid-syslog-example" swVersion="0.4.0" enterpriseId="32473" ip="10.0.2.15"][logPipeline@32473 transport="mtls" collectorAuth="fingerprint+chain" atRest="aes-256-gcm"][solidsyslog@67478 source="StreamSender" category="SENDER_DELIVERY_RESTORED" detail="DELIVERY_RESTORED"] ﻿SolidSyslog event
parsed PRIORITY=45 TIMESTAMP=2026-11-08T15:48:56+00:00 HOSTNAME=10.0.2.15 APP_NAME=solid-syslog-example PROCID= MSGID=SENDER_DELIVERY_RESTORED STRUCTURED_DATA=[meta sequenceId="5" sysUpTime="922"][timeQuality tzKnown="1" isSynced="0"][origin software="solid-syslog-example" swVersion="0.4.0" enterpriseId="32473" ip="10.0.2.15"][logPipeline@32473 transport="mtls" collectorAuth="fingerprint+chain" atRest="aes-256-gcm"][solidsyslog@67478 source="StreamSender" category="SENDER_DELIVERY_RESTORED" detail="DELIVERY_RESTORED"] MSG=SolidSyslog event
wire   <134>1 2026-11-08T15:48:58.820000Z 10.0.2.15 solid-syslog-example - BOOT [meta sequenceId="6" sysUpTime="1182"][timeQuality tzKnown="1" isSynced="0"][origin software="solid-syslog-example" swVersion="0.4.0" enterpriseId="32473" ip="10.0.2.15"][logPipeline@32473 transport="mtls" collectorAuth="fingerprint+chain" atRest="aes-256-gcm"] ﻿device started
parsed PRIORITY=134 TIMESTAMP=2026-11-08T15:48:58+00:00 HOSTNAME=10.0.2.15 APP_NAME=solid-syslog-example PROCID= MSGID=BOOT STRUCTURED_DATA=[meta sequenceId="6" sysUpTime="1182"][timeQuality tzKnown="1" isSynced="0"][origin software="solid-syslog-example" swVersion="0.4.0" enterpriseId="32473" ip="10.0.2.15"][logPipeline@32473 transport="mtls" collectorAuth="fingerprint+chain" atRest="aes-256-gcm"] MSG=device started
```

**RESULT: PASS**
