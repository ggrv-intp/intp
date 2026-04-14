/*
 * mbw.c -- resctrl Memory Bandwidth Reader (V6 eBPF)
 *
 * IntP metric: mbw (memory bandwidth utilization)
 * Equivalent to: print_mem_report() in v1-original/intp.stp
 *
 * This is the userspace resctrl reader for the V6 eBPF variant.
 * Same logic as v4-hybrid-procfs/src/mbw.c -- reads MBM counters from
 * the resctrl filesystem.
 *
 * Kernel interface:
 *   /sys/fs/resctrl/mon_groups/intp_mon_<PID>/mon_data/mon_L3_XX/mbm_total_bytes
 *
 * Formula:
 *   delta_bytes = mbm_total_now - mbm_total_prev
 *   bw_mbps = delta_bytes / interval_sec / 1000000
 *   utilization = bw_mbps / max_bw_mbps * 100
 *
 * Assumes shared/intp-resctrl-helper.sh has created the monitoring group.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char mbm_path[256];
static long max_bw_mbps;
static unsigned long long prev_mbm_bytes;

/*
 * resctrl_mbw_init -- Initialize resctrl MBM reader
 *
 * Parameters:
 *   pid        - Target PID (for finding resctrl group name)
 *   max_bw     - Maximum memory bandwidth in MB/s
 *
 * Returns: 0 on success, -1 on error
 */
int resctrl_mbw_init(pid_t pid, long max_bw)
{
    /* TODO: Build path to mbm_total_bytes file */
    /* TODO: Find mon_L3_XX directory */
    /* TODO: Read initial value */
    (void)pid;
    max_bw_mbps = max_bw;
    (void)mbm_path;
    (void)prev_mbm_bytes;
    return 0;
}

/*
 * resctrl_mbw_sample -- Read MBM counter and compute utilization
 *
 * Returns: Memory bandwidth utilization as percentage (0.0 - 100.0)
 */
double resctrl_mbw_sample(void)
{
    /* TODO: Read current mbm_total_bytes */
    /* TODO: Compute delta, normalize to max bandwidth */
    /* TODO: Store current as previous */
    (void)max_bw_mbps;
    return 0.0;
}
