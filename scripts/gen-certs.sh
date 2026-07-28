#!/usr/bin/env bash
# The test secrets for ./run.sh: one CA, a collector server certificate and its
# fingerprint, the certificate the collector renews to and its fingerprint, a
# broker server certificate, a device client certificate for mTLS, and the
# device's provisioned symmetric keys.
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

# The address the device reaches everything on: QEMU's slirp gateway. It must
# appear as a SAN or the device's hostname verification rejects the peer.
GATEWAY_IP="10.0.2.2"

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
    -extfile <(printf 'subjectAltName=IP:%s\nextendedKeyUsage=serverAuth\n' "$GATEWAY_IP") 2>/dev/null

# --- the collector's fingerprint ---------------------------------------------
# RFC 5425 section 4.2.2 form: the IANA hash name, a colon, then the SHA-256 of the
# DER certificate as colon-separated hex. A device that pins its collector is given
# this at provisioning, out of band; a fresh PKI each run means a fresh pin with it.
{
    printf 'sha-256:'
    openssl x509 -in collector.crt -noout -fingerprint -sha256 | cut -d= -f2
} > collector.pin

# --- the collector's renewed certificate -------------------------------------
# Same CA and the same name, a new key: what the collector presents after it
# renews. A renewal changes the fingerprint, so a device that pins the collector
# needs this one before the collector switches to it.
newkey collector-next.key
openssl req -new -key collector-next.key \
    -subj "/O=solid-syslog-example/CN=collector" -out collector-next.csr 2>/dev/null
openssl x509 -req -in collector-next.csr -CA ca.crt -CAkey ca.key -CAcreateserial \
    -days "$DAYS" -sha256 -out collector-next.crt \
    -extfile <(printf 'subjectAltName=IP:%s\nextendedKeyUsage=serverAuth\n' "$GATEWAY_IP") 2>/dev/null
{
    printf 'sha-256:'
    openssl x509 -in collector-next.crt -noout -fingerprint -sha256 | cut -d= -f2
} > collector-next.pin

# --- the broker (server) -----------------------------------------------------
# The system the device already speaks mTLS to, before SolidSyslog exists. Its
# own certificate rather than the collector's, because it is a different peer.
newkey broker.key
openssl req -new -key broker.key \
    -subj "/O=solid-syslog-example/CN=broker" -out broker.csr 2>/dev/null
openssl x509 -req -in broker.csr -CA ca.crt -CAkey ca.key -CAcreateserial \
    -days "$DAYS" -sha256 -out broker.crt \
    -extfile <(printf 'subjectAltName=IP:%s\nextendedKeyUsage=serverAuth\n' "$GATEWAY_IP") 2>/dev/null

# --- the device (client, for mTLS) -------------------------------------------
newkey device.key
openssl req -new -key device.key \
    -subj "/O=solid-syslog-example/CN=solid-syslog-example-device" -out device.csr 2>/dev/null
openssl x509 -req -in device.csr -CA ca.crt -CAkey ca.key -CAcreateserial \
    -days "$DAYS" -sha256 -out device.crt \
    -extfile <(printf 'extendedKeyUsage=clientAuth\n') 2>/dev/null

# --- the device's provisioned symmetric keys ---------------------------------
# A secured device holds symmetric key material as well as a PKI — storage
# protection, anti-rollback counters, sealed configuration. Provisioned at
# manufacture, so they exist before any software asks for one.
for name in device-storage log-store; do
    openssl rand -out "${name}.key" 32
done

rm -f collector.csr collector-next.csr broker.csr device.csr ca.srl
chmod 644 ./*.crt ./*.key ./*.pin

echo "PKI in ${OUT}: $(ls *.crt *.key *.pin | tr '\n' ' ')"
