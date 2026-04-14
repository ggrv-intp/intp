/*
 * cpu.c -- CPU Utilization (V4 hybrid)
 *
 * IntP metric: cpu (CPU utilization)
 * Equivalent to: print_cpu_report() in v1-original/intp.stp
 *
 * Kernel interface:
 *   /proc/stat
 *
 * /proc/stat format (first line):
 *   cpu  user nice system idle iowait irq softirq steal guest guest_nice
 *
 * All values are in "jiffies" (USER_HZ, typically 100 Hz = 10ms ticks).
 *
 * Formula:
 *   active = delta(user + system)
 *   total  = delta(user + nice + system + idle + iowait + irq + softirq + steal)
 *   utilization = active / total * 100
 *
 * For per-process CPU utilization, read /proc/<PID>/stat instead:
 *   Fields 14 (utime) and 15 (stime) in clock ticks.
 *   per_proc_util = delta(utime + stime) / (interval_sec * CLK_TCK) * 100
 *
 * The original intp.stp uses scheduler tracepoints (sched_switch) to track
 * per-CPU time attribution. The /proc/stat approach gives equivalent
 * system-wide utilization. For per-process, /proc/<PID>/stat is used.
 *
 * Tradeoffs vs V1:
 *   - Equivalent for system-wide CPU utilization
 *   - Per-process uses /proc/<PID>/stat (polling, not event-driven)
 *   - Cannot provide per-core breakdown without reading per-cpu lines
 *   - Simpler: single file read vs scheduler probe instrumentation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static pid_t monitored_pid;
static unsigned long long prev_utime;
static unsigned long long prev_stime;
static long clk_tck;

/*
 * read_proc_pid_stat -- Read utime and stime from /proc/<PID>/stat
 *
 * Fields in /proc/<PID>/stat (1-indexed):
 *   14 = utime (user mode ticks)
 *   15 = stime (kernel mode ticks)
 *
 * Returns: 0 on success, -1 on error
 */
static int read_proc_pid_stat(pid_t pid,
                              unsigned long long *utime,
                              unsigned long long *stime)
{
    /* TODO: Open /proc/<PID>/stat */
    /* TODO: Parse fields 14 and 15 */
    /* Note: field 2 (comm) may contain spaces and parens, so parse carefully */
    /* Strategy: find last ')' then count fields from there */
    (void)pid;
    (void)utime;
    (void)stime;
    return -1;
}

/*
 * cpu_init -- Initialize CPU utilization collector
 *
 * Parameters:
 *   target_pid - PID to monitor
 *
 * Returns: 0 on success, -1 on error
 */
int cpu_init(pid_t target_pid)
{
    /* TODO: Store PID, get CLK_TCK, read initial values */
    monitored_pid = target_pid;
    clk_tck = sysconf(_SC_CLK_TCK);

    read_proc_pid_stat(monitored_pid, &prev_utime, &prev_stime);
    return 0;
}

/*
 * cpu_sample -- Read current CPU ticks and compute utilization
 *
 * Returns: CPU utilization as percentage (0.0 - 100.0)
 *          Based on per-process utime+stime delta
 */
double cpu_sample(void)
{
    /* TODO: Read current utime and stime */
    /* TODO: Compute delta_ticks = (utime - prev_utime) + (stime - prev_stime) */
    /* TODO: utilization = delta_ticks / (interval_sec * clk_tck) * 100 */
    /* TODO: Store current as previous */
    return 0.0;
}
