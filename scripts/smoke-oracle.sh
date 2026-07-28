#!/usr/bin/env bash
# Prove every listener the run depends on, before the device runs.
#
# A listener that syslog-ng parsed is not a listener that works: a wrong path in a
# tls() block, a certificate without the right SAN, a peer-verify that quietly
# accepts nobody — all of those start cleanly and fail only when a device finally
# tries to connect, several stages later. This sends one record over each transport
# and checks it arrived, so the failure lands here instead.
#
# The device's broker is proved for a different reason. The baseline holds an mTLS
# session to it for the whole run, and what that session costs is what SolidSyslog
# is then NOT charged for. A dead broker would read as "the baseline uses less
# memory" rather than as a failure, so it is proved here too.
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
# unauthenticated clients and the stages that rely on it are not proving what they
# claim to prove.
#
# Only meaningful next to the positive check above. On its own it also passes when
# there is no listener at all — nothing arrives either way. The pair is what says
# "6515 is up and it enforces".
send_mtls_nocert() {
    framed MTLSNOCERT | timeout 10 openssl s_client -connect "${HOST}:6515" \
        -CAfile "${CERTS}/ca.crt" -quiet -no_ign_eof >/dev/null 2>&1
}

# The broker speaks no syslog, so its evidence is the reversed echo it sends back
# rather than a line in the collector's log — proof the session carried bytes,
# not merely that a handshake completed.
BROKER_PROBE=BROKER
BROKER_ECHO=REKORB

broker_echo() { # extra s_client args, e.g. the client certificate
    printf '%s\n' "$BROKER_PROBE" | timeout 10 openssl s_client -connect "${HOST}:8883" \
        -CAfile "${CERTS}/ca.crt" -verify_return_error -quiet "$@" 2>/dev/null
}

send_udp || true
send_tcp || true
send_tls || true
send_mtls || true
send_mtls_nocert || true

# depends_on guarantees the broker container was started, not that s_server has
# bound its socket, so the first probe doubles as the wait. Retrying the real
# check rather than polling the port separately keeps one mechanism: if it never
# answers, this fails exactly as it would have anyway. The certless probe follows
# it, so it runs against a broker already known to be up — which is the only way
# a refusal proves anything.
broker_authenticated=""
for _ in $(seq 1 10); do
    broker_authenticated="$(broker_echo -cert "${CERTS}/device.crt" -key "${CERTS}/device.key")"
    if grep -qxF "$BROKER_ECHO" <<<"$broker_authenticated"; then
        break
    fi
    sleep 1
done
broker_certless="$(broker_echo)"

# Let syslog-ng flush all four before reading back.
sleep 2
received="$(cat "$SMOKE_LOG" 2>/dev/null || true)"

failures=0
check() { # $1 = haystack, $2 = label, $3 = port, $4 = expected line
    if grep -qxF "$4" <<<"$1"; then
        printf '  OK    %-6s %s\n' "$2" "$3"
    else
        printf '  FAIL  %-6s %s — nothing came back\n' "$2" "$3"
        failures=$((failures + 1))
    fi
}

refuse() { # $1 = haystack, $2 = label, $3 = port, $4 = line that must NOT be there
    if grep -qxF "$4" <<<"$1"; then
        printf '  FAIL  %-6s %s — accepted a client with no certificate\n' "$2" "$3"
        failures=$((failures + 1))
    else
        printf '  OK    %-6s %s — refused a client with no certificate\n' "$2" "$3"
    fi
}

check  "$received" udp  5514 "UDP"
check  "$received" tcp  5601 "TCP"
check  "$received" tls  6514 "TLS"
check  "$received" mtls 6515 "MTLS"
refuse "$received" mtls 6515 "MTLSNOCERT"

check  "$broker_authenticated" broker 8883 "$BROKER_ECHO"
refuse "$broker_certless"      broker 8883 "$BROKER_ECHO"

exit "$failures"
