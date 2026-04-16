/*
 * cpu.c -- CPU Utilization (V4 hybrid)
 *
 * Per-process CPU utilization from /proc/<pid>/stat (fields 14 utime, 15 stime).
 * Normalizes delta-ticks over wall-clock delta * CLK_TCK.
 *
 * Field 2 (comm) may contain spaces and parentheses, so we locate the last ')'
 * in the buffer and index fields from there.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include "intp.h"

static pid_t monitored_pid;
static unsigned long long prev_utime;
static unsigned long long prev_stime;
static struct timespec    prev_ts;
static long               clk_tck;
static int                ok;

static int read_proc_pid_stat(pid_t pid,
                              unsigned long long *utime,
                              unsigned long long *stime)
{
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/stat", (int)pid);

    FILE *f = fopen(path, "r");
    if (!f)
        return -1;

    char buf[4096];
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    if (n == 0)
        return -1;
    buf[n] = '\0';

    char *rp = strrchr(buf, ')');
    if (!rp)
        return -1;
    rp++;

    unsigned long long ut = 0, st = 0;
    int matched = sscanf(rp,
        " %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %llu %llu",
        &ut, &st);
    if (matched != 2)
        return -1;

    *utime = ut;
    *stime = st;
    return 0;
}

int cpu_init(pid_t target_pid)
{
    monitored_pid = target_pid;
    clk_tck = sysconf(_SC_CLK_TCK);
    if (clk_tck <= 0)
        clk_tck = 100;

    if (read_proc_pid_stat(monitored_pid, &prev_utime, &prev_stime) < 0) {
        ok = 0;
        return -1;
    }
    clock_gettime(CLOCK_MONOTONIC, &prev_ts);
    ok = 1;
    return 0;
}

double cpu_sample(void)
{
    if (!ok)
        return NAN;

    unsigned long long ut = 0, st = 0;
    if (read_proc_pid_stat(monitored_pid, &ut, &st) < 0)
        return NAN;

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    double dt = (now.tv_sec  - prev_ts.tv_sec)
              + (now.tv_nsec - prev_ts.tv_nsec) / 1e9;
    if (dt <= 0.0)
        return 0.0;

    unsigned long long d_ticks = (ut - prev_utime) + (st - prev_stime);

    prev_utime = ut;
    prev_stime = st;
    prev_ts    = now;

    return (d_ticks / (dt * (double)clk_tck)) * 100.0;
}
