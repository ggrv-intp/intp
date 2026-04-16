/*
 * netp.c -- Network Physical Utilization (V4 hybrid)
 *
 * Reads /sys/class/net/<iface>/statistics/{tx,rx}_bytes. Delta over wall-
 * clock interval, normalized by NIC line rate. sysfs counters are the same
 * source the kernel uses, so fidelity equals V1's counter-based netp.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "intp.h"

static char              tx_path[256];
static char              rx_path[256];
static long              nic_speed_bps;
static unsigned long long prev_tx;
static unsigned long long prev_rx;
static struct timespec    prev_ts;
static int                ok;

static unsigned long long read_sysfs_ull(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f)
        return 0;
    unsigned long long v = 0;
    if (fscanf(f, "%llu", &v) != 1)
        v = 0;
    fclose(f);
    return v;
}

int netp_init(const char *iface, long nic_speed_mbps)
{
    snprintf(tx_path, sizeof(tx_path),
             "/sys/class/net/%.63s/statistics/tx_bytes", iface);
    snprintf(rx_path, sizeof(rx_path),
             "/sys/class/net/%.63s/statistics/rx_bytes", iface);

    nic_speed_bps = (nic_speed_mbps > 0 ? nic_speed_mbps : 1000)
                    * 1000000L / 8;

    FILE *tf = fopen(tx_path, "r");
    FILE *rf = fopen(rx_path, "r");
    if (!tf || !rf) {
        if (tf) fclose(tf);
        if (rf) fclose(rf);
        ok = 0;
        return -1;
    }
    fclose(tf);
    fclose(rf);

    prev_tx = read_sysfs_ull(tx_path);
    prev_rx = read_sysfs_ull(rx_path);
    clock_gettime(CLOCK_MONOTONIC, &prev_ts);
    ok = 1;
    return 0;
}

double netp_sample(void)
{
    if (!ok)
        return NAN;

    unsigned long long tx = read_sysfs_ull(tx_path);
    unsigned long long rx = read_sysfs_ull(rx_path);

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    double dt = (now.tv_sec  - prev_ts.tv_sec)
              + (now.tv_nsec - prev_ts.tv_nsec) / 1e9;
    if (dt <= 0.0)
        return 0.0;

    unsigned long long d_bytes = (tx - prev_tx) + (rx - prev_rx);

    prev_tx = tx;
    prev_rx = rx;
    prev_ts = now;

    double util = (double)d_bytes / ((double)nic_speed_bps * dt) * 100.0;
    if (util < 0.0)   util = 0.0;
    if (util > 100.0) util = 100.0;
    return util;
}
