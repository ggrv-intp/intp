/*
 * llcmr.c -- LLC Miss Ratio (V4 hybrid, perf_event_open)
 *
 * Opens two PERF_TYPE_HW_CACHE counters on the target PID:
 *   LL | OP_READ | RESULT_ACCESS  (references)
 *   LL | OP_READ | RESULT_MISS    (misses)
 *
 * inherit=1 so child threads count. Requires CAP_PERFMON (or root, or
 * perf_event_paranoid <= 1). On failure we degrade to NaN.
 *
 * HW_CACHE generic events translate to the platform's native PMU event codes:
 *   - Intel:   last-level cache references / misses
 *   - AMD EPYC/Zen: L3 generic events
 *   - ARM:     PMU LLC counters (availability varies by SoC)
 * Portability issue: some ARM SoCs do not expose HW_CACHE_LL; in that case
 * perf_event_open returns ENOENT and we disable the metric.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <linux/perf_event.h>
#include "intp.h"

static int fd_refs = -1;
static int fd_miss = -1;
static unsigned long long prev_refs;
static unsigned long long prev_miss;

static long sys_perf_event_open(struct perf_event_attr *attr,
                                pid_t pid, int cpu, int group_fd,
                                unsigned long flags)
{
    return syscall(__NR_perf_event_open, attr, pid, cpu, group_fd, flags);
}

static int setup_cache_counter(pid_t pid, unsigned long long cache_config)
{
    struct perf_event_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.type            = PERF_TYPE_HW_CACHE;
    attr.size            = sizeof(attr);
    attr.config          = cache_config;
    attr.disabled        = 1;
    attr.exclude_hv      = 1;
    attr.inherit         = 1;

    int fd = (int)sys_perf_event_open(&attr, pid, -1, -1, 0);
    if (fd < 0)
        return -1;

    if (ioctl(fd, PERF_EVENT_IOC_RESET, 0) < 0 ||
        ioctl(fd, PERF_EVENT_IOC_ENABLE, 0) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

static unsigned long long read_counter(int fd)
{
    unsigned long long v = 0;
    if (read(fd, &v, sizeof(v)) != (ssize_t)sizeof(v))
        return 0;
    return v;
}

int llcmr_init(pid_t target_pid)
{
    unsigned long long refs_config =
        (unsigned long long)PERF_COUNT_HW_CACHE_LL |
        ((unsigned long long)PERF_COUNT_HW_CACHE_OP_READ << 8) |
        ((unsigned long long)PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16);

    unsigned long long miss_config =
        (unsigned long long)PERF_COUNT_HW_CACHE_LL |
        ((unsigned long long)PERF_COUNT_HW_CACHE_OP_READ << 8) |
        ((unsigned long long)PERF_COUNT_HW_CACHE_RESULT_MISS << 16);

    fd_refs = setup_cache_counter(target_pid, refs_config);
    fd_miss = setup_cache_counter(target_pid, miss_config);

    if (fd_refs < 0 || fd_miss < 0) {
        fprintf(stderr,
                "[llcmr] perf_event_open failed (%s) -- metric disabled "
                "(need CAP_PERFMON or perf_event_paranoid <= 1)\n",
                strerror(errno));
        llcmr_cleanup();
        return -1;
    }

    prev_refs = read_counter(fd_refs);
    prev_miss = read_counter(fd_miss);
    return 0;
}

double llcmr_sample(void)
{
    if (fd_refs < 0 || fd_miss < 0)
        return NAN;

    unsigned long long refs = read_counter(fd_refs);
    unsigned long long miss = read_counter(fd_miss);

    unsigned long long d_refs = refs - prev_refs;
    unsigned long long d_miss = miss - prev_miss;

    prev_refs = refs;
    prev_miss = miss;

    if (d_refs == 0)
        return 0.0;

    double ratio = (double)d_miss * 100.0 / (double)d_refs;
    if (ratio < 0.0)   ratio = 0.0;
    if (ratio > 100.0) ratio = 100.0;
    return ratio;
}

void llcmr_cleanup(void)
{
    if (fd_refs >= 0) close(fd_refs);
    if (fd_miss >= 0) close(fd_miss);
    fd_refs = -1;
    fd_miss = -1;
}
