/*
 * llcocc.c -- LLC Occupancy (V4 hybrid, resctrl)
 *
 * Sums llc_occupancy across every mon_L3_* domain in our monitoring group,
 * normalizes by total LLC bytes across all domains. On multi-socket (Intel)
 * or multi-CCX (AMD) systems this correctly reflects aggregate occupancy.
 *
 * This metric is the headline fix for kernel 6.8 -- where V1 broke when
 * cqm_rmid was removed from struct hw_perf_event. resctrl is the kernel's
 * supported replacement and is vendor-neutral.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "intp.h"
#include "resctrl.h"

static char  occ_paths[RESCTRL_MAX_DOMAINS][RESCTRL_PATH_MAX];
static int   n_domains;
static long  llc_total_bytes;
static int   ok;

int llcocc_init(pid_t target_pid, long llc_size_kb)
{
    (void)target_pid;  /* group setup was done before us, in main() */

    n_domains = resctrl_enumerate_domains("llc_occupancy",
                                          occ_paths, RESCTRL_MAX_DOMAINS);
    if (n_domains == 0 || llc_size_kb <= 0) {
        fprintf(stderr, "[llcocc] no llc_occupancy counters or unknown LLC size "
                        "-- metric disabled\n");
        ok = 0;
        return -1;
    }

    /* detect_llc_size_kb() gave us cpu0's LLC; multiply by domain count. */
    llc_total_bytes = llc_size_kb * 1024L * n_domains;
    ok = 1;
    return 0;
}

double llcocc_sample(void)
{
    if (!ok || llc_total_bytes <= 0)
        return NAN;

    unsigned long long bytes = resctrl_sum_counters(occ_paths, n_domains);
    double pct = (double)bytes / (double)llc_total_bytes * 100.0;

    if (pct < 0.0)   pct = 0.0;
    if (pct > 100.0) pct = 100.0;
    return pct;
}
