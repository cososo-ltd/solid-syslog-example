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
[report] flash_text,354584,349992,4592
[report] flash_data,448,316,132
[report] static_bss,112656,110876,1780
[report] heap_used,4440,4440,0
[report] mbedtls_peak,21316,21332,-16
[report] mbedtls_free,11452,11436,16
[report] lwip_mem_free,7576,7576,0
[report] lwip_pbufs_free,14,14,0
[report] stack_log,1024,120,904
[report] stack_service,52,52,0
[report] stack_harness,2848,2840,8
[report] --- end ---
[device] ready
```

### Size cross-check

```text
   text	   data	    bss	    dec	    hex	filename
 354576	    456	 112656	 467688	  722e8	/w/build/baseline-cross/baseline.elf
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
parsed PRIORITY=134 TIMESTAMP=2026-08-15T13:02:30+00:00 HOSTNAME=localhost APP_NAME= PROCID= MSGID=BOOT STRUCTURED_DATA= MSG=device started
```

## Self-check (vs measurements/udp.csv)

```text
  OK    flash_text: 354584 (expected 354584, Δ0)
  OK    flash_data: 448 (expected 448, Δ0)
  OK    static_bss: 112656 (expected 112656, Δ0)
  OK    heap_used: 4440 (expected 4440, Δ0)
  OK    mbedtls_peak: 21316 (expected 21236, Δ80)
  OK    mbedtls_free: 11452 (expected 11532, Δ80)
  OK    lwip_mem_free: 7576 (expected 7576, Δ0)
  OK    lwip_pbufs_free: 14 (expected 14, Δ0)
  OK    stack_log: 1024 (expected 1024, Δ0)
  OK    stack_service: 52 (expected 52, Δ0)
  OK    stack_harness: 2848 (expected 2848, Δ0)
```

**RESULT: PASS**
