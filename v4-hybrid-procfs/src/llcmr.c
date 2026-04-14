/*
 * llcmr.c -- LLC Miss Ratio (V4 hybrid)
 *
 * IntP metric: llcmr (Last-Level Cache miss ratio)
 * Equivalent to: print_cache_report() in v1-original/intp.stp
 *
 * Kernel interface:
 *   perf_event_open() syscall with PERF_TYPE_HW_CACHE config
 *
 * This module uses the perf_event_open() syscall to program hardware
 * performance counters for LLC cache references and cache misses.
 * The perf subsystem handles the per-CPU multiplexing and provides
 * a stable interface across kernel versions (since Linux 2.6.31).
 *
 * perf_event_attr configuration:
 *   type = PERF_TYPE_HW_CACHE
 *   config for LLC loads:
 *     (PERF_COUNT_HW_CACHE_LL) |
 *     (PERF_COUNT_HW_CACHE_OP_READ << 8) |
 *     (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16)
 *   config for LLC misses:
 *     (PERF_COUNT_HW_CACHE_LL) |
 *     (PERF_COUNT_HW_CACHE_OP_READ << 8) |
 *     (PERF_COUNT_HW_CACHE_RESULT_MISS << 16)
 *
 * Formula:
 *   miss_ratio = (delta_misses * 100) / delta_references
 *   If delta_references == 0, miss_ratio = 0
 *
 * The original intp.stp uses the same perf_event infrastructure but
 * accesses it through SystemTap's perf_event integration. The V4
 * approach calls perf_event_open() directly, which is functionally
 * equivalent but without the SystemTap dependency.
 *
 * Requirements:
 *   - CAP_PERFMON capability, or
 *   - /proc/sys/kernel/perf_event_paranoid <= 1, or
 *   - Root privileges
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/perf_event.h>

static int fd_refs = -1;   /* File descriptor for LLC references counter */
static int fd_miss = -1;   /* File descriptor for LLC misses counter */
static unsigned long long prev_refs;
static unsigned long long prev_miss;

/*
 * perf_event_open wrapper
 */
static long sys_perf_event_open(struct perf_event_attr *attr,
                                pid_t pid, int cpu, int group_fd,
                                unsigned long flags)
{
    return syscall(__NR_perf_event_open, attr, pid, cpu, group_fd, flags);
}

/*
 * setup_cache_counter -- Configure a perf_event for HW_CACHE events
 *
 * Reference perf_event_attr setup:
 *
 *   struct perf_event_attr attr = {
 *       .type           = PERF_TYPE_HW_CACHE,
 *       .size           = sizeof(struct perf_event_attr),
 *       .config         = cache_config,
 *       .disabled       = 1,
 *       .exclude_kernel = 0,  // include kernel-space cache events
 *       .exclude_hv     = 1,
 *       .inherit        = 1,  // count child processes too
 *   };
 *   int fd = perf_event_open(&attr, target_pid, -1, -1, 0);
 *
 * Returns: file descriptor on success, -1 on error
 */
static int setup_cache_counter(pid_t pid, unsigned long long cache_config)
{
    /* TODO: Set up perf_event_attr */
    /* TODO: Call perf_event_open() */
    /* TODO: Enable the counter with ioctl(fd, PERF_EVENT_IOC_ENABLE, 0) */
    (void)pid;
    (void)cache_config;
    return -1;
}

/*
 * read_counter -- Read the current value of a perf_event counter
 *
 * Returns: counter value, or 0 on error
 */
static unsigned long long read_counter(int fd)
{
    /* TODO: read(fd, &value, sizeof(value)) */
    (void)fd;
    return 0;
}

/*
 * llcmr_init -- Initialize LLC miss ratio collector
 *
 * Parameters:
 *   target_pid - PID to monitor (perf_event will track this process)
 *
 * Returns: 0 on success, -1 on error
 */
int llcmr_init(pid_t target_pid)
{
    /* TODO: Set up LLC references counter */
    unsigned long long refs_config =
        (PERF_COUNT_HW_CACHE_LL) |
        ((unsigned long long)PERF_COUNT_HW_CACHE_OP_READ << 8) |
        ((unsigned long long)PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16);

    /* TODO: Set up LLC misses counter */
    unsigned long long miss_config =
        (PERF_COUNT_HW_CACHE_LL) |
        ((unsigned long long)PERF_COUNT_HW_CACHE_OP_READ << 8) |
        ((unsigned long long)PERF_COUNT_HW_CACHE_RESULT_MISS << 16);

    fd_refs = setup_cache_counter(target_pid, refs_config);
    fd_miss = setup_cache_counter(target_pid, miss_config);

    if (fd_refs < 0 || fd_miss < 0) {
        /* TODO: Handle error, possibly fall back to PERF_TYPE_HARDWARE */
        return -1;
    }

    prev_refs = read_counter(fd_refs);
    prev_miss = read_counter(fd_miss);

    return 0;
}

/*
 * llcmr_sample -- Read counters and compute LLC miss ratio
 *
 * Returns: LLC miss ratio as percentage (0.0 - 100.0)
 */
double llcmr_sample(void)
{
    /* TODO: Read current counter values */
    /* TODO: Compute deltas */
    /* TODO: miss_ratio = delta_miss * 100.0 / delta_refs */
    /* TODO: Handle delta_refs == 0 (return 0.0) */
    /* TODO: Store current as previous */
    return 0.0;
}

/*
 * llcmr_cleanup -- Close perf_event file descriptors
 */
void llcmr_cleanup(void)
{
    /* TODO: Close fd_refs and fd_miss */
    if (fd_refs >= 0) close(fd_refs);
    if (fd_miss >= 0) close(fd_miss);
    fd_refs = -1;
    fd_miss = -1;
}
