/*
 * mbw.c -- Memory Bandwidth Utilization (V4 hybrid)
 *
 * IntP metric: mbw (memory bandwidth utilization)
 * Equivalent to: print_mem_report() in v1-original/intp.stp
 *
 * Kernel interface (primary):
 *   /sys/fs/resctrl/mon_data/mon_L3_XX/mbm_total_bytes
 *
 * The resctrl filesystem exposes Intel RDT Memory Bandwidth Monitoring (MBM)
 * counters. The mbm_total_bytes file reports the total bytes of memory
 * traffic (LLC-to-DRAM) for the monitored group.
 *
 * Formula:
 *   delta_bytes = mbm_total_now - mbm_total_prev
 *   bw_mbps = delta_bytes / interval_sec / 1000000
 *   utilization = bw_mbps / max_bw_mbps * 100
 *
 * The original intp.stp uses perf_event for the same MBM counters via the
 * perf subsystem. The resctrl approach is equivalent but accessed through
 * the filesystem instead of perf_event_open().
 *
 * Fallback (without resctrl):
 *   Use perf_event_open() with uncore IMC (Integrated Memory Controller)
 *   events. This requires CAP_PERFMON and knowledge of the specific IMC
 *   PMU event codes, which vary by CPU generation:
 *     - Intel: uncore_imc_X/cas_count_read/ + uncore_imc_X/cas_count_write/
 *     - AMD: amd_df/DRAM channel controller events
 *   The uncore approach is more complex and less portable than resctrl.
 *
 * Tradeoffs vs V1:
 *   - Equivalent fidelity when resctrl is available
 *   - Requires Intel RDT MBM or AMD PQoS MBM hardware support
 *   - Without resctrl, the uncore fallback is CPU-generation-specific
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char mbm_path[256];
static long max_bw_mbps;
static unsigned long long prev_mbm_bytes;

static unsigned long long read_resctrl_ull(const char *path)
{
    /* TODO: Read unsigned long long from resctrl file */
    (void)path;
    return 0;
}

/*
 * find_mon_l3_dir -- Find the first mon_L3_XX directory in resctrl
 *
 * Returns: 0 on success (mbm_path populated), -1 on error
 */
static int find_mon_l3_dir(void)
{
    /* TODO: Iterate /sys/fs/resctrl/mon_data/mon_L3_* */
    /* TODO: Find first directory with mbm_total_bytes file */
    /* TODO: Store path in mbm_path */
    return -1;
}

/*
 * mbw_init -- Initialize memory bandwidth utilization collector
 *
 * Parameters:
 *   max_bw - Maximum memory bandwidth in MB/s (for normalization)
 *
 * Returns: 0 on success, -1 on error
 */
int mbw_init(long max_bw)
{
    /* TODO: Find resctrl mon_L3 directory */
    /* TODO: Read initial mbm_total_bytes */
    max_bw_mbps = max_bw;
    find_mon_l3_dir();
    prev_mbm_bytes = read_resctrl_ull(mbm_path);
    return 0;
}

/*
 * mbw_sample -- Read current MBM counter and compute utilization
 *
 * Returns: Memory bandwidth utilization as percentage (0.0 - 100.0)
 */
double mbw_sample(void)
{
    /* TODO: Read current mbm_total_bytes */
    /* TODO: Compute delta from previous sample */
    /* TODO: Convert to MB/s: delta / interval_sec / 1000000 */
    /* TODO: Normalize to max bandwidth: bw_mbps / max_bw_mbps * 100 */
    /* TODO: Store current as previous */
    return 0.0;
}
