/*
 * nets.c -- Network Stack Utilization (V4 hybrid, POLLING APPROXIMATION)
 *
 * V1's nets measures kernel network-stack service time by timestamping each
 * packet at dev_queue_xmit()/netif_receive_skb(). Without kernel probes, V4
 * cannot replicate that. This module approximates stack pressure as packet
 * rate normalized by line-rate-implied max PPS (using 64-byte packets as
 * worst-case). This is correlated with but NOT equivalent to service time:
 *   - small-packet-heavy workloads read higher (correct: stack is busier)
 *   - large-packet bulk transfer reads lower (correct: fewer skbuffs)
 *   - queue-congestion events invisible to counters are missed (caveat)
 *
 * Fidelity classification: ~  (see docs/VARIANT-COMPARISON.md).
 * Plot it alongside V1's nets to expose the approximation gap quantitatively.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "intp.h"

static char               iface_name[64];
static unsigned long long prev_rx_packets;
static unsigned long long prev_tx_packets;
static struct timespec    prev_ts;
static long               max_pps;
static int                ok;

static int read_iface_packets(const char *iface,
                              unsigned long long *rx_p,
                              unsigned long long *tx_p)
{
    char path[256];
    snprintf(path, sizeof(path),
             "/sys/class/net/%.63s/statistics/rx_packets", iface);
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    if (fscanf(f, "%llu", rx_p) != 1) { fclose(f); return -1; }
    fclose(f);

    snprintf(path, sizeof(path),
             "/sys/class/net/%.63s/statistics/tx_packets", iface);
    f = fopen(path, "r");
    if (!f) return -1;
    if (fscanf(f, "%llu", tx_p) != 1) { fclose(f); return -1; }
    fclose(f);
    return 0;
}

int nets_init(const char *iface)
{
    snprintf(iface_name, sizeof(iface_name), "%.63s", iface);

    long mbps = detect_nic_speed(iface_name);
    /* Worst-case PPS: line-rate with minimum-size (64B) frames.
     * Mbps * 1e6 / 8 bytes = bytes/s ; / 64 = pps */
    max_pps = mbps * 1000000L / 8 / 64;
    if (max_pps <= 0)
        max_pps = 1;

    if (read_iface_packets(iface_name, &prev_rx_packets, &prev_tx_packets) < 0) {
        ok = 0;
        return -1;
    }
    clock_gettime(CLOCK_MONOTONIC, &prev_ts);
    ok = 1;
    return 0;
}

double nets_sample(void)
{
    if (!ok)
        return NAN;

    unsigned long long rx_p = 0, tx_p = 0;
    if (read_iface_packets(iface_name, &rx_p, &tx_p) < 0)
        return NAN;

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    double dt = (now.tv_sec  - prev_ts.tv_sec)
              + (now.tv_nsec - prev_ts.tv_nsec) / 1e9;
    if (dt <= 0.0)
        return 0.0;

    double pps = (double)((rx_p - prev_rx_packets) + (tx_p - prev_tx_packets)) / dt;

    prev_rx_packets = rx_p;
    prev_tx_packets = tx_p;
    prev_ts         = now;

    double util = pps / (double)max_pps * 100.0;
    if (util < 0.0)   util = 0.0;
    if (util > 100.0) util = 100.0;
    return util;
}
