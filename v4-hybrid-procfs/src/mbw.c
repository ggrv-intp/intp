/*
 * mbw.c -- Memory Bandwidth Utilization (V4 hybrid, resctrl)
 *
 * Sums mbm_total_bytes across every mon_L3_* domain in our monitoring group
 * (Intel sockets, AMD CCX, or ARM MPAM partitions), computes delta-over-wall-
 * clock to get MB/s, normalizes by the platform's max memory bandwidth.
 *
 * Caveats documented by Sohal et al. (RTNS 2022) and Intel errata SKX99/BDF102:
 *   - Counter accuracy is 5%+ on validated platforms, much worse on some
 *     Skylake/Broadwell SKUs (kernel applies correction factors).
 *   - "MBM may report up to 2x theoretical bandwidth" on affected hardware.
 * We cap to 100% for [0,100] semantics. Raw mode is a future switch.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "intp.h"
#include "resctrl.h"

static char               mbm_paths[RESCTRL_MAX_DOMAINS][RESCTRL_PATH_MAX];
static int                n_domains;
static long               max_bw_mbps_val;
static unsigned long long prev_bytes;
static struct timespec    prev_ts;
static int                ok;

int mbw_init(long max_bw_mbps)
{
    max_bw_mbps_val = (max_bw_mbps > 0) ? max_bw_mbps : 42656;

    n_domains = resctrl_enumerate_domains("mbm_total_bytes",
                                          mbm_paths, RESCTRL_MAX_DOMAINS);
    if (n_domains == 0) {
        fprintf(stderr, "[mbw] no mbm_total_bytes counters found -- "
                        "metric disabled\n");
        ok = 0;
        return -1;
    }

    prev_bytes = resctrl_sum_counters(mbm_paths, n_domains);
    clock_gettime(CLOCK_MONOTONIC, &prev_ts);
    ok = 1;
    return 0;
}

double mbw_sample(void)
{
    if (!ok)
        return NAN;

    unsigned long long now_bytes = resctrl_sum_counters(mbm_paths, n_domains);

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    double dt = (now.tv_sec  - prev_ts.tv_sec)
              + (now.tv_nsec - prev_ts.tv_nsec) / 1e9;
    if (dt <= 0.0)
        return 0.0;

    /* bytes / sec / 1e6 = MB/s  -->  pct = MB/s / max_MB/s * 100 */
    double mbps = (double)(now_bytes - prev_bytes) / dt / 1e6;
    double util = mbps / (double)max_bw_mbps_val * 100.0;

    prev_bytes = now_bytes;
    prev_ts    = now;

    if (util < 0.0)   util = 0.0;
    if (util > 100.0) util = 100.0;
    return util;
}
