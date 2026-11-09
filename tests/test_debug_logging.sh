#!/usr/bin/env bash

set -euo pipefail

debug_port="19737"
error_port="19738"
info_port="19739"
debug_log="/tmp/network-midterm-debug.log"
error_log="/tmp/network-midterm-error.log"
info_log="/tmp/network-midterm-info.log"
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

assert_complete_log_lines() {
    local log_file="$1"

    if grep -Evq '^(server listening on |\[(ERROR|INFO|DEBUG) pid=[0-9]+\] )' "${log_file}"; then
        echo "log records were interleaved or malformed" >&2
        return 1
    fi
}

./build/server --host 127.0.0.1 --port "${debug_port}" \
    --idle-timeout 10 --log-level debug >"${debug_log}" 2>&1 &
server_pid=$!
wait_for_server "${debug_port}"
printf '2\n3\n4\n' | ./build/client 127.0.0.1 "${debug_port}" >/dev/null
sleep 0.2
cleanup
server_pid=""

grep -q '\[INFO ' "${debug_log}"
grep -q '\[DEBUG ' "${debug_log}"
grep -Eq 'client_connected peer=\[127\.0\.0\.1\]:[0-9]+ child=[0-9]+' "${debug_log}"
grep -q 'action=GET_SYSTEM_INFO' "${debug_log}"
grep -q 'action=GET_STATS' "${debug_log}"
grep -q 'action=QUIT' "${debug_log}"
grep -q 'client_disconnected peer=' "${debug_log}"
assert_complete_log_lines "${debug_log}"

./build/server --host 127.0.0.1 --port "${info_port}" \
    --idle-timeout 10 >"${info_log}" 2>&1 &
server_pid=$!
wait_for_server "${info_port}"
printf '4\n' | ./build/client 127.0.0.1 "${info_port}" >/dev/null
sleep 0.2
cleanup
server_pid=""

grep -q '\[INFO ' "${info_log}"
grep -Eq 'client_connected peer=\[127\.0\.0\.1\]:[0-9]+ child=[0-9]+' "${info_log}"
grep -q 'action=QUIT' "${info_log}"
assert_complete_log_lines "${info_log}"
if grep -q '\[DEBUG ' "${info_log}"; then
    echo "default info log level unexpectedly emitted debug records" >&2
    exit 1
fi

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
echo "runtime-log-level-info: PASS"
echo "runtime-log-level-error: PASS"
