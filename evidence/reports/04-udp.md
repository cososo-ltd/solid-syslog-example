# solid-syslog-example — run (udp)

## Device (self-measured)

```text
[device] solid-syslog-example (FreeRTOS + lwIP + mbedTLS + FatFs)
[device] starting simulated existing application...
[sim] broker session to 10.0.2.2:8883: TLSv1.3, TLS1-3-CHACHA20-POLY1305-SHA256
[device]   sim app (lwIP up, FatFs mounted, broker session held over mTLS): ready
[device]   first record logged: yes
[report] --- SolidSyslog cost above baseline (simulated existing application) ---
[report] key,current,baseline,used_above_baseline
[report] flash_text,363916,356996,6920
[report] flash_data,372,236,136
[report] static_bss,114140,111452,2688
[report] heap_used,4440,4440,0
[report] mbedtls_peak,21008,20996,12
[report] mbedtls_free,11760,11772,-12
[report] lwip_mem_free,7576,7576,0
[report] lwip_pbufs_free,13,13,0
[report] stack_log,1036,116,920
[report] stack_service,56,56,0
[report] stack_harness,3048,3040,8
[report] --- end ---
[device]   records logged: 1
[device] ready
```

### Size cross-check

```text
   text	   data	    bss	    dec	    hex	filename
 363908	    380	 114140	 478428	  74cdc	/w/build/baseline-cross/baseline.elf
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
wire   <134>1 - - - - BOOT - ﻿device started
parsed PRIORITY=134 TIMESTAMP=2026-11-27T22:51:30+00:00 HOSTNAME=localhost APP_NAME= PROCID= MSGID=BOOT STRUCTURED_DATA= MSG=device started
```

**RESULT: PASS**
