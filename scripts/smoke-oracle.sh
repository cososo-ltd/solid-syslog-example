#!/usr/bin/env bash
# Prove every oracle listener, before any tag depends on one.
#
# A listener that syslog-ng parsed is not a listener that works: a wrong path in a
# tls() block, a certificate without the right SAN, a peer-verify that quietly
# accepts nobody — all of those start cleanly and fail only when a device finally
# tries to connect, several tags later. This sends one record over each transport
# and checks it arrived, so the failure lands here instead.
#
# Runs in the `run` container, which shares the oracle's network namespace, so the
# listeners are on localhost. Records carry app-name "oracle-smoke", which
# syslog-ng routes to smoke.log and away from the received_*.log the run reports.
set -uo pipefail

CERTS="${CERTS:-/w/build/certs}"
SMOKE_LOG="${ORACLE_LOG_DIR:-/collector}/smoke.log"
HOST=127.0.0.1

# MSGID names the transport; the template writes it out with the peer CN beside it.
record() {
    printf '<134>1 2026-01-01T00:00:00.000000Z smoke-host oracle-smoke - %s - listener check' "$1"
}

# RFC 6587 octet counting, which is what syslog() expects on a stream transport.
framed() {
    local message
    message="$(record "$1")"
    printf '%d %s' "${#message}" "$message"
}

send_udp() { record UDP > "/dev/udp/${HOST}/5514"; }
send_tcp() { framed TCP > "/dev/tcp/${HOST}/5601"; }

send_tls() {
    framed TLS | timeout 10 openssl s_client -connect "${HOST}:6514" \
        -CAfile "${CERTS}/ca.crt" -verify_return_error -quiet -no_ign_eof >/dev/null 2>&1
}

send_mtls() {
    framed MTLS | timeout 10 openssl s_client -connect "${HOST}:6515" \
        -CAfile "${CERTS}/ca.crt" -verify_return_error -quiet -no_ign_eof \
        -cert "${CERTS}/device.crt" -key "${CERTS}/device.key" >/dev/null 2>&1
}

# The negative case, and the reason mTLS has a port to itself: a client presenting
# no certificate must be turned away. If this record arrives, 6515 is accepting
# unauthenticated clients and the tags that rely on it are not proving what they
# claim to prove.
send_mtls_nocert() {
    framed MTLSNOCERT | timeout 10 openssl s_client -connect "${HOST}:6515" \
        -CAfile "${CERTS}/ca.crt" -quiet -no_ign_eof >/dev/null 2>&1
}

send_udp || true
send_tcp || true
send_tls || true
send_mtls || true
send_mtls_nocert || true

# Let syslog-ng flush all four before reading back.
sleep 2
received="$(cat "$SMOKE_LOG" 2>/dev/null || true)"

failures=0
check() { # $1 = label, $2 = port, $3 = expected line
    if grep -qxF "$3" <<<"$received"; then
        printf '  OK    %-5s %s\n' "$1" "$2"
    else
        printf '  FAIL  %-5s %s — no record arrived\n' "$1" "$2"
        failures=$((failures + 1))
    fi
}

refuse() { # $1 = label, $2 = port, $3 = line that must NOT be there
    if grep -qxF "$3" <<<"$received"; then
        printf '  FAIL  %-5s %s — accepted a client with no certificate\n' "$1" "$2"
        failures=$((failures + 1))
    else
        printf '  OK    %-5s %s — refused a client with no certificate\n' "$1" "$2"
    fi
}

check  udp  5514 "UDP"
check  tcp  5601 "TCP"
check  tls  6514 "TLS"
check  mtls 6515 "MTLS"
refuse mtls 6515 "MTLSNOCERT"

exit "$failures"
