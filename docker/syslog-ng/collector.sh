#!/bin/bash
# Runs syslog-ng for ./run.sh, serving the collector's certificate from
# /run/collector so it can be renewed mid-run.
#
# A renewal is what a site does when the collector's certificate is replaced: the
# new certificate goes in and the service restarts, dropping every open session.
# The device asks for one by writing /collector-state/renew over semihosting, and
# waits for /collector-state/renewed.
set -u

mkdir -p /run/collector /collector-state
rm -f /collector-state/renew /collector-state/renewed

serve() {
    cp "/certs/$1.key" /run/collector/collector.key
    cp "/certs/$1.crt" /run/collector/collector.crt
    /usr/local/bin/entrypoint.sh "${@:2}" &
    pid=$!
    until syslog-ng-ctl stats > /dev/null 2>&1; do sleep 0.1; done
}

serve collector "$@"
while kill -0 "$pid" 2> /dev/null; do
    if [ -f /collector-state/renew ]; then
        syslog-ng-ctl stop > /dev/null
        wait "$pid"
        serve collector-next "$@"
        rm -f /collector-state/renew
        touch /collector-state/renewed
    fi
    sleep 0.2
done
