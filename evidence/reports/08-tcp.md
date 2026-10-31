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
[report] flash_text,363048,352804,10244
[report] flash_data,496,320,176
[report] static_bss,119216,111136,8080
[report] heap_used,4440,4440,0
[report] mbedtls_peak,21356,21360,-4
[report] mbedtls_free,11412,11408,4
[report] lwip_mem_free,7576,7576,0
[report] lwip_pbufs_free,14,13,1
[report] stack_log,788,116,672
[report] stack_service,472,56,416
[report] stack_harness,2848,2840,8
[report] --- end ---
[device]   records logged: 1
[device] ready
```

### Size cross-check

```text
   text	   data	    bss	    dec	    hex	filename
 363040	    504	 119216	 482760	  75dc8	/w/build/baseline-cross/baseline.elf
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
wire   <134>1 2026-10-31T13:30:24.850000Z 10.0.2.15 solid-syslog-example - BOOT [meta sequenceId="1"] ﻿device started
parsed PRIORITY=134 TIMESTAMP=2026-10-31T13:30:24+00:00 HOSTNAME=10.0.2.15 APP_NAME=solid-syslog-example PROCID= MSGID=BOOT STRUCTURED_DATA=[meta sequenceId="1"] MSG=device started
```

**RESULT: PASS**
