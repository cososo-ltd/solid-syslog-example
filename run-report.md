# solid-syslog-example — run (linked)

## Device (self-measured)

```text
[device] solid-syslog-example (FreeRTOS + lwIP + mbedTLS + FatFs)
[device] starting simulated existing application...
[sim] broker session to 10.0.2.2:8883: TLSv1.3, TLS1-3-CHACHA20-POLY1305-SHA256
[device]   sim app (lwIP up, FatFs mounted, broker session held over mTLS): ready
[report] --- SolidSyslog cost above baseline (simulated existing application) ---
[report] key,current,baseline,used_above_baseline
[report] flash_text,349992,349992,0
[report] flash_data,316,316,0
[report] static_bss,110876,110876,0
[report] heap_used,4440,4440,0
[report] mbedtls_peak,21328,21332,-4
[report] mbedtls_free,11440,11436,4
[report] lwip_mem_free,7576,7576,0
[report] lwip_pbufs_free,14,14,0
[report] stack_log,120,120,0
[report] stack_service,52,52,0
[report] stack_harness,2840,2840,0
[report] --- end ---
[device] ready
```

### Size cross-check

```text
   text	   data	    bss	    dec	    hex	filename
 349984	    324	 110876	 461184	  70980	/w/build/baseline-cross/baseline.elf
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

## Self-check (vs measurements/linked.csv)

```text
  OK    flash_text: 349992 (expected 349992, Δ0)
  OK    flash_data: 316 (expected 316, Δ0)
  OK    static_bss: 110876 (expected 110876, Δ0)
  OK    heap_used: 4440 (expected 4440, Δ0)
  OK    mbedtls_peak: 21328 (expected 21332, Δ4)
  OK    mbedtls_free: 11440 (expected 11436, Δ4)
  OK    lwip_mem_free: 7576 (expected 7576, Δ0)
  OK    lwip_pbufs_free: 14 (expected 14, Δ0)
  OK    stack_log: 120 (expected 120, Δ0)
  OK    stack_service: 52 (expected 52, Δ0)
  OK    stack_harness: 2840 (expected 2840, Δ0)
```

**RESULT: PASS**
