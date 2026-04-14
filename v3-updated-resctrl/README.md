# V3 -- Updated IntP with Resctrl LLC Solution

This variant restores all 7/7 metrics on kernel 6.8+ by using the resctrl
(Resource Control) filesystem as a companion data source for LLC occupancy.

## How It Works

The resctrl filesystem (`/sys/fs/resctrl`) exposes Intel RDT (Resource
Director Technology) and AMD PQoS monitoring data. Instead of reading the
`cqm_rmid` field from kernel internals, this variant:

1. Creates a resctrl monitoring group for the target PID
2. Reads LLC occupancy from `/sys/fs/resctrl/mon_groups/<name>/mon_data/mon_L3_XX/llc_occupancy`
3. A companion shell daemon (`shared/intp-resctrl-helper.sh`) manages the
   resctrl group lifecycle

The SystemTap script reads the resctrl value via embedded C file I/O instead
of MSR access.

## Metrics Status

| Metric | Status | Source |
|--------|--------|--------|
| netp   | Working | SystemTap kernel probes |
| nets   | Working | SystemTap kernel probes |
| blk    | Working | SystemTap kernel probes |
| mbw    | Working | SystemTap perf events |
| llcmr  | Working | SystemTap perf events |
| llcocc | **Working** | resctrl filesystem |
| cpu    | Working | SystemTap kernel probes |

## Files

- `intp-resctrl.stp` -- SystemTap script with resctrl integration
- `install/install_ubuntu24_desktop.md` -- Ubuntu 24.04 installation guide
- `docs/LLC-OCCUPANCY-RESCTRL.md` -- Technical details of the resctrl approach
- `docs/RESCTRL-VALIDATION.md` -- Validation methodology and results

## Usage

```bash
# Start the resctrl helper daemon
sudo ../shared/intp-resctrl-helper.sh start <PID>

# Run the profiler
sudo stap -g intp-resctrl.stp <PID> <interval_ms>

# Stop the helper when done
sudo ../shared/intp-resctrl-helper.sh stop
```

## Requirements

- Linux kernel 6.8+
- SystemTap with guru mode
- Kernel debuginfo
- Intel RDT or AMD PQoS hardware support
- resctrl filesystem mounted (`mount -t resctrl resctrl /sys/fs/resctrl`)

---

> TODO: Populate with files from the old dev branch.
> Use: git show old-dev:intp-resctrl.stp > intp-resctrl.stp (etc.)
