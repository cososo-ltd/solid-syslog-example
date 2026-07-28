# solid-syslog-example — run (error-handler)

## Device (self-measured)

```text
[device] solid-syslog-example (FreeRTOS + lwIP + mbedTLS + FatFs)
[device] starting simulated existing application...
[sim] broker session to 10.0.2.2:8883: TLSv1.3, TLS1-3-CHACHA20-POLY1305-SHA256
[device]   sim app (lwIP up, FatFs mounted, broker session held over mTLS): ready
[report] --- SolidSyslog cost above baseline (simulated existing application) ---
[report] key,current,baseline,used_above_baseline
[report] flash_text,350392,349992,400
[report] flash_data,320,316,4
[report] static_bss,110880,110876,4
[report] heap_used,4440,4440,0
[report] mbedtls_peak,21208,21332,-124
[report] mbedtls_free,11560,11436,124
[report] lwip_mem_free,7576,7576,0
[report] lwip_pbufs_free,13,14,-1
[report] stack_log,120,120,0
[report] stack_service,52,52,0
[report] stack_harness,2840,2840,0
[report] --- end ---
[device] ready
```

### Size cross-check

```text
   text	   data	    bss	    dec	    hex	filename
 350384	    328	 110880	 461592	  70b18	/w/build/baseline-cross/baseline.elf
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
(nothing — this device sends no records yet)
```

## Self-check (vs measurements/error-handler.csv)

```text
  OK    flash_text: 350392 (expected 350392, Δ0)
  OK    flash_data: 320 (expected 320, Δ0)
  OK    static_bss: 110880 (expected 110880, Δ0)
  OK    heap_used: 4440 (expected 4440, Δ0)
  OK    mbedtls_peak: 21208 (expected 21336, Δ128)
  OK    mbedtls_free: 11560 (expected 11432, Δ128)
  OK    lwip_mem_free: 7576 (expected 7576, Δ0)
  OK    lwip_pbufs_free: 13 (expected 14, Δ1)
  OK    stack_log: 120 (expected 120, Δ0)
  OK    stack_service: 52 (expected 52, Δ0)
  OK    stack_harness: 2840 (expected 2840, Δ0)
```

**RESULT: PASS**
