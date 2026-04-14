#!/bin/bash
# -----------------------------------------------------------------------------
# run-intp-bpftrace.sh -- IntP bpftrace Orchestrator (V5)
#
# Runs bpftrace scripts for software metrics and the resctrl helper for
# hardware metrics (mbw, llcocc) in parallel. Aggregates output in
# IntP-compatible format.
#
# Equivalent to: sudo stap -g intp.stp <PID> <interval_ms>
#
# Usage: sudo ./run-intp-bpftrace.sh <PID> <interval_ms>
#
# Architecture:
#   - bpftrace intp.bt runs in background collecting software metrics
#   - resctrl helper runs in background providing mbw and llcocc
#   - This script reads from both sources and produces combined output
# -----------------------------------------------------------------------------

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SHARED_DIR="$(cd "$SCRIPT_DIR/../shared" && pwd)"
RESCTRL_HELPER="$SHARED_DIR/intp-resctrl-helper.sh"

usage() {
    echo "Usage: $0 <PID> <interval_ms>"
    echo ""
    echo "Arguments:"
    echo "  PID          Target process ID to monitor"
    echo "  interval_ms  Sampling interval in milliseconds (default: 1000)"
    exit 1
}

cleanup() {
    echo "Stopping IntP bpftrace profiler..."
    # TODO: Kill background bpftrace process
    # TODO: Stop resctrl helper
    if [ -n "${BPFTRACE_PID:-}" ]; then
        kill "$BPFTRACE_PID" 2>/dev/null || true
        wait "$BPFTRACE_PID" 2>/dev/null || true
    fi
    "$RESCTRL_HELPER" stop 2>/dev/null || true
    echo "Stopped."
}

# -- Argument parsing ----------------------------------------------------------

TARGET_PID="${1:-}"
INTERVAL_MS="${2:-1000}"

if [ -z "$TARGET_PID" ]; then
    echo "Error: PID required"
    usage
fi

if ! kill -0 "$TARGET_PID" 2>/dev/null; then
    echo "Error: Process $TARGET_PID does not exist"
    exit 1
fi

# -- Check dependencies --------------------------------------------------------

if ! command -v bpftrace >/dev/null 2>&1; then
    echo "Error: bpftrace not found. Install with: apt install bpftrace"
    exit 1
fi

if [ ! -f /sys/kernel/btf/vmlinux ]; then
    echo "Warning: BTF not available. Some probes may not work."
    echo "Rebuild kernel with CONFIG_DEBUG_INFO_BTF=y"
fi

# -- Set up resctrl for hardware metrics ---------------------------------------

echo "Setting up resctrl monitoring group for PID $TARGET_PID..."
"$RESCTRL_HELPER" start "$TARGET_PID" || {
    echo "Warning: resctrl setup failed. mbw and llcocc will return 0."
}

trap cleanup EXIT INT TERM

# -- Start bpftrace for software metrics ---------------------------------------

echo "Starting bpftrace (software metrics)..."
# TODO: Start bpftrace intp.bt in background
# bpftrace "$SCRIPT_DIR/intp.bt" -p "$TARGET_PID" &
# BPFTRACE_PID=$!

# -- Main aggregation loop -----------------------------------------------------

INTERVAL_SEC=$(echo "scale=3; $INTERVAL_MS / 1000" | bc)

echo "# IntP bpftrace profiler -- PID $TARGET_PID, interval ${INTERVAL_MS}ms"
echo "# netp  nets  blk  mbw  llcmr  llcocc  cpu"

while kill -0 "$TARGET_PID" 2>/dev/null; do
    sleep "$INTERVAL_SEC"

    # TODO: Read software metrics from bpftrace output
    # TODO: Read hardware metrics from resctrl
    MBW=$("$RESCTRL_HELPER" read-mbw 2>/dev/null || echo 0)
    LLCOCC=$("$RESCTRL_HELPER" read-llcocc 2>/dev/null || echo 0)

    # TODO: Combine and output in IntP format
    # printf "%.2f  %.2f  %.2f  %.2f  %.2f  %.2f  %.2f\n" \
    #     "$NETP" "$NETS" "$BLK" "$MBW" "$LLCMR" "$LLCOCC" "$CPU"

    echo "TODO: aggregate bpftrace output with resctrl values (mbw=$MBW llcocc=$LLCOCC)"
done

echo "# Target process $TARGET_PID exited"
