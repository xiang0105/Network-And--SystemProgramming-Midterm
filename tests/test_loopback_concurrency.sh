#!/usr/bin/env bash

set -euo pipefail

port="19736"
server_log="/tmp/network-midterm-server-smoke.log"
server_pid=""
client_pids=()

cleanup() {
    local pid

    for pid in "${client_pids[@]:-}"; do
        kill "${pid}" 2>/dev/null || true
        wait "${pid}" 2>/dev/null || true
    done
    if [[ -n "${server_pid}" ]]; then
        kill "${server_pid}" 2>/dev/null || true
        wait "${server_pid}" 2>/dev/null || true
    fi
}

trap cleanup EXIT

./build/server --host 127.0.0.1 --port "${port}" --idle-timeout 10 \
    >"${server_log}" 2>&1 &
server_pid=$!

for _ in $(seq 1 30); do
    if nc -z 127.0.0.1 "${port}" 2>/dev/null; then
        break
    fi
    sleep 0.1
done

client_output=$(printf '2\n3\n4\n' | ./build/client 127.0.0.1 "${port}")
grep -q 'kernel=' <<<"${client_output}"
grep -q 'completed=0' <<<"${client_output}"

for _ in $(seq 1 10); do
    (sleep 5 | nc 127.0.0.1 "${port}" >/dev/null) &
    client_pids+=("$!")
done

sleep 1
mapfile -t child_pids < <(pgrep -P "${server_pid}")
if (( ${#child_pids[@]} < 10 )); then
    echo "expected at least 10 concurrent child processes, found ${#child_pids[@]}" >&2
    exit 1
fi

echo "server_pid=${server_pid}"
echo "concurrent_children=${#child_pids[@]}"
ps -o pid=,ppid=,stat=,cmd= -p "$(IFS=,; echo "${child_pids[*]}")"

for pid in "${client_pids[@]}"; do
    wait "${pid}"
done
client_pids=()

echo "loopback-client-smoke: PASS"
echo "loopback-10-client-concurrency: PASS"
