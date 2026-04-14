/*
 * llcocc.c -- LLC Occupancy (V4 hybrid)
 *
 * IntP metric: llcocc (Last-Level Cache occupancy)
 * Equivalent to: print_llc_report() in v1-original/intp.stp
 *
 * Kernel interface:
 *   /sys/fs/resctrl/mon_groups/<name>/mon_data/mon_L3_XX/llc_occupancy
 *
 * The resctrl filesystem exposes Intel RDT Cache Monitoring Technology (CMT)
 * data. The llc_occupancy file reports the number of bytes of LLC occupied
 * by the monitored process group.
 *
 * This is the metric that broke in kernel 6.8: the original intp.stp
 * accessed the cqm_rmid field from struct hw_perf_event to read the
 * process RMID, then queried the QOS_L3_OCC MSR directly. Kernel 6.8
 * removed cqm_rmid, breaking that approach.
 *
 * The resctrl approach is the kernel's official replacement interface.
 * It requires creating a monitoring group and assigning the target PID
 * to it (handled by shared/intp-resctrl-helper.sh or programmatically).
 *
 * Formula:
 *   occupancy_bytes = read(llc_occupancy)
 *   occupancy_pct = occupancy_bytes / llc_total_size_bytes * 100
 *
 * The llc_occupancy value is already in bytes. Normalization requires
 * knowing the total LLC size, which can be detected from:
 *   /sys/devices/system/cpu/cpu0/cache/indexN/size (where N = highest level)
 *
 * Requirements:
 *   - Intel RDT CMT or AMD PQoS L3 Monitoring hardware
 *   - resctrl filesystem mounted
 *   - Monitoring group created for target PID
 *
 * Tradeoffs vs V1:
 *   - Equivalent data (resctrl uses the same hardware counters)
 *   - More robust (filesystem vs direct MSR access)
 *   - Requires explicit group management (intp-resctrl-helper.sh)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char llcocc_path[256];
static long llc_total_bytes;

/*
 * llcocc_init -- Initialize LLC occupancy collector
 *
 * Parameters:
 *   target_pid  - PID being monitored (for finding resctrl group)
 *   llc_size_kb - Total LLC size in KB (for normalization)
 *
 * Returns: 0 on success, -1 on error
 *
 * Assumes that shared/intp-resctrl-helper.sh has already created a
 * monitoring group named "intp_mon_<PID>".
 */
int llcocc_init(pid_t target_pid, long llc_size_kb)
{
    /* TODO: Build path to llc_occupancy file */
    /* Path pattern: /sys/fs/resctrl/mon_groups/intp_mon_<PID>/mon_data/mon_L3_XX/llc_occupancy */
    /* TODO: Find the mon_L3_XX directory (iterate, pick first) */
    (void)target_pid;

    llc_total_bytes = llc_size_kb * 1024L;

    return 0;
}

/*
 * llcocc_sample -- Read LLC occupancy and compute percentage
 *
 * Returns: LLC occupancy as percentage of total LLC (0.0 - 100.0)
 */
double llcocc_sample(void)
{
    /* TODO: Read llc_occupancy file (single unsigned long long value) */
    /* TODO: Normalize: occupancy_bytes / llc_total_bytes * 100 */
    /* TODO: Handle llc_total_bytes == 0 (return 0.0) */
    (void)llcocc_path;
    (void)llc_total_bytes;
    return 0.0;
}
