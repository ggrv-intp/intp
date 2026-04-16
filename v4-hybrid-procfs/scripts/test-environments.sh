#!/usr/bin/env bash
# test-environments.sh -- run an IntP V4 capture under bare-metal, container,
# and (manually) VM, comparing the metrics each environment can supply.
#
# Usage: ./test-environments.sh <workload-command> [duration-seconds]
#
# Outputs:   results-YYYYMMDD-HHMMSS/{baremetal,container}.tsv
#            results-YYYYMMDD-HHMMSS/comparison.md (if compare script exists)

set -euo pipefail

if [[ $# -lt 1 ]]; then
    echo "Usage: $0 <workload-command> [duration-seconds]" >&2
    exit 1
fi

WORKLOAD=$1
DURATION=${2:-60}
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
ROOT_DIR=$(cd "$SCRIPT_DIR/.." && pwd)
INTP=${INTP:-$ROOT_DIR/intp-hybrid}
OUTDIR=$ROOT_DIR/results-$(date +%Y%m%d-%H%M%S)

mkdir -p "$OUTDIR"
echo "==> output dir: $OUTDIR"

if [[ ! -x "$INTP" ]]; then
    echo "intp-hybrid binary not found at $INTP -- run 'make' first" >&2
    exit 1
fi

# 1. bare-metal
echo "==> bare-metal capture ($DURATION s)"
"$INTP" --interval 1 --duration "$DURATION" --output tsv \
    > "$OUTDIR/baremetal.tsv" &
INTP_PID=$!
( eval "$WORKLOAD" ) &
WORK_PID=$!
wait "$INTP_PID" 2>/dev/null || true
kill "$WORK_PID" 2>/dev/null || true
wait "$WORK_PID" 2>/dev/null || true

# 2. container (Docker)
if command -v docker >/dev/null 2>&1; then
    echo "==> container capture ($DURATION s)"
    docker run --rm --privileged \
        --pid=host \
        -v /sys/fs/resctrl:/sys/fs/resctrl \
        -v "$ROOT_DIR":/intp \
        -v "$OUTDIR":/out \
        ubuntu:24.04 bash -lc "
            apt-get update >/dev/null 2>&1 || true
            /intp/intp-hybrid --interval 1 --duration $DURATION --output tsv \
                > /out/container.tsv &
            INTP_PID=\$!
            ( $WORKLOAD ) &
            WORK_PID=\$!
            wait \$INTP_PID
            kill \$WORK_PID 2>/dev/null || true
        "
else
    echo "==> docker not present; skipping container capture"
fi

# 3. VM (manual)
echo "==> VM capture is manual; see DESIGN.md 'Cross-environment behavior'"
echo "    suggested: virt-install with --feature pmu=on, then run intp-hybrid inside"

if [[ -x "$SCRIPT_DIR/compare-environments.py" ]]; then
    "$SCRIPT_DIR/compare-environments.py" "$OUTDIR" \
        > "$OUTDIR/comparison.md" || true
fi

echo "==> done. Results: $OUTDIR"
