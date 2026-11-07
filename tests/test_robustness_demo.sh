#!/usr/bin/env bash

set -euo pipefail

demo_executable="${1:-./build/robustness_demo}"
output=$("${demo_executable}")

grep -q '^case=fragmented-input unsafe=FAIL(short-read) robust=PASS(reassembled)$' <<<"${output}"
grep -q '^case=incomplete-input unsafe=HANG(blocked) robust=PASS(timeout)$' <<<"${output}"
grep -q '^case=closed-peer unsafe=CRASH(SIGPIPE) robust=PASS(error-returned)$' <<<"${output}"
grep -q '^robustness-comparison: PASS$' <<<"${output}"

printf '%s\n' "${output}"
