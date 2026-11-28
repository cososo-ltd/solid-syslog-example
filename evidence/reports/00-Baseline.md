# solid-syslog-example — run (Baseline)

## Device (self-measured)

```text
[device] solid-syslog-example (FreeRTOS + lwIP + mbedTLS + FatFs)
[device] starting simulated existing application...
[sim] broker session to 10.0.2.2:8883: TLSv1.3, TLS1-3-CHACHA20-POLY1305-SHA256
[device]   sim app (lwIP up, FatFs mounted, broker session held over mTLS): ready
[report] --- SolidSyslog cost above baseline (simulated existing application) ---
[report] key,current,baseline,used_above_baseline
[report] flash_text,356996,356996,0
[report] flash_data,236,236,0
[report] static_bss,111452,111452,0
[report] heap_used,4440,4440,0
[report] mbedtls_peak,21008,20996,12
[report] mbedtls_free,11760,11772,-12
[report] lwip_mem_free,7576,7576,0
[report] lwip_pbufs_free,13,13,0
[report] stack_log,116,116,0
[report] stack_service,56,56,0
[report] stack_harness,3040,3040,0
[report] --- end ---
[device]   records logged: 0
[device] ready
```

### Size cross-check

```text
   text	   data	    bss	    dec	    hex	filename
 356988	    244	 111452	 468684	  726cc	/w/build/baseline-cross/baseline.elf
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

**RESULT: PASS**
