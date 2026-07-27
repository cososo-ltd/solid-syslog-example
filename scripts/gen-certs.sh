#!/usr/bin/env bash
# The test PKI for ./run.sh: one CA, a collector server certificate, and a device
# client certificate for mTLS.
#
# Regenerated on every run and never committed. Nothing here is a secret worth
# keeping, and a fresh PKI each run is what stops the device quietly passing
# because a certificate happened to be lying around from last time.
#
# P-256 rather than RSA throughout. It is what a device this size actually uses:
# smaller certificates to store, a cheaper handshake, and it keeps the key type
# in the example consistent with the one an integrator would choose.
#
# NOT A MODEL FOR PRODUCTION PKI. A real deployment issues the device identity
# during manufacture or provisioning, into a secure element or protected flash,
# and never has the private key sitting in a file next to the certificate.
set -euo pipefail

OUT="${1:-/w/build/certs}"
DAYS=3650

# The address the device reaches the collector on: QEMU's slirp gateway. It must
# appear as a SAN or the device's hostname verification rejects the collector.
COLLECTOR_IP="10.0.2.2"

mkdir -p "$OUT"
cd "$OUT"

newkey() { openssl genpkey -algorithm EC -pkeyopt ec_paramgen_curve:P-256 -out "$1" 2>/dev/null; }

# --- the CA ------------------------------------------------------------------
newkey ca.key
openssl req -x509 -new -key ca.key -sha256 -days "$DAYS" \
    -subj "/O=solid-syslog-example/CN=solid-syslog-example test CA" \
    -out ca.crt 2>/dev/null

# --- the collector (server) --------------------------------------------------
newkey collector.key
openssl req -new -key collector.key \
    -subj "/O=solid-syslog-example/CN=collector" -out collector.csr 2>/dev/null
openssl x509 -req -in collector.csr -CA ca.crt -CAkey ca.key -CAcreateserial \
    -days "$DAYS" -sha256 -out collector.crt \
    -extfile <(printf 'subjectAltName=IP:%s\nextendedKeyUsage=serverAuth\n' "$COLLECTOR_IP") 2>/dev/null

# --- the device (client, for mTLS) -------------------------------------------
newkey device.key
openssl req -new -key device.key \
    -subj "/O=solid-syslog-example/CN=solid-syslog-example-device" -out device.csr 2>/dev/null
openssl x509 -req -in device.csr -CA ca.crt -CAkey ca.key -CAcreateserial \
    -days "$DAYS" -sha256 -out device.crt \
    -extfile <(printf 'extendedKeyUsage=clientAuth\n') 2>/dev/null

rm -f collector.csr device.csr ca.srl
chmod 644 ./*.crt ./*.key

echo "PKI in ${OUT}: $(ls *.crt *.key | tr '\n' ' ')"
