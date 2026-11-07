#!/usr/bin/env bash

set -euo pipefail

debug_port="19737"
error_port="19738"
debug_log="/tmp/network-midterm-debug.log"
error_log="/tmp/network-midterm-error.log"
server_pid=""

cleanup() {
    if [[ -n "${server_pid}" ]]; then
        kill "${server_pid}" 2>/dev/null || true
        wait "${server_pid}" 2>/dev/null || true
    fi
}

trap cleanup EXIT

wait_for_server() {
    local port="$1"

    for _ in $(seq 1 30); do
        if nc -z 127.0.0.1 "${port}" 2>/dev/null; then
            return 0
        fi
        sleep 0.1
    done
    return 1
}

./build/server --host 127.0.0.1 --port "${debug_port}" \
    --idle-timeout 10 --log-level debug >"${debug_log}" 2>&1 &
server_pid=$!
wait_for_server "${debug_port}"
printf '4\n' | ./build/client 127.0.0.1 "${debug_port}" >/dev/null
sleep 0.2
cleanup
server_pid=""

grep -q '\[INFO ' "${debug_log}"
grep -q '\[DEBUG ' "${debug_log}"

./build/server --host 127.0.0.1 --port "${error_port}" \
    --idle-timeout 10 --log-level error >"${error_log}" 2>&1 &
server_pid=$!
wait_for_server "${error_port}"
printf '4\n' | ./build/client 127.0.0.1 "${error_port}" >/dev/null
sleep 0.2
cleanup
server_pid=""

if grep -Eq '\[(INFO|DEBUG) ' "${error_log}"; then
    echo "error log level unexpectedly emitted info or debug records" >&2
    exit 1
fi

echo "runtime-log-level-debug: PASS"
echo "runtime-log-level-error: PASS"
