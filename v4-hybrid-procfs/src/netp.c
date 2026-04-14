/*
 * netp.c -- Network Physical Utilization (V4 hybrid)
 *
 * IntP metric: netp (network physical utilization)
 * Equivalent to: print_netphy_report() in v1-original/intp.stp
 *
 * Kernel interface:
 *   /sys/class/net/<iface>/statistics/tx_bytes
 *   /sys/class/net/<iface>/statistics/rx_bytes
 *
 * Formula:
 *   delta_bytes = (tx_bytes_now - tx_bytes_prev) + (rx_bytes_now - rx_bytes_prev)
 *   utilization = delta_bytes / (nic_speed_bytes_per_sec * interval_sec) * 100
 *
 * Constants:
 *   NIC speed normalization: 1 Gbps = 125000000 bytes/sec (1000 * 1000 * 1000 / 8)
 *   This matches the hardcoded constant 125000000 in the original intp.stp.
 *
 * Tradeoffs vs V1:
 *   - Equivalent fidelity: sysfs counters are the same source the kernel uses
 *   - Simpler: no kernel probes needed, just read two files
 *   - The original intp.stp also uses counters (not per-packet), so no loss
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char tx_path[256];
static char rx_path[256];
static long nic_speed_bps;  /* bytes per second */
static unsigned long long prev_tx;
static unsigned long long prev_rx;

static unsigned long long read_sysfs_ull(const char *path)
{
    /* TODO: Implement sysfs file read */
    /* Open path, read unsigned long long, close, return value */
    /* Return 0 on error */
    (void)path;
    return 0;
}

/*
 * netp_init -- Initialize network physical utilization collector
 *
 * Parameters:
 *   iface          - Network interface name (e.g., "eth0", "ens33")
 *   nic_speed_mbps - NIC speed in Mbps (e.g., 1000 for 1 Gbps)
 *
 * Returns: 0 on success, -1 on error
 */
int netp_init(const char *iface, long nic_speed_mbps)
{
    /* TODO: Build sysfs paths and read initial values */
    snprintf(tx_path, sizeof(tx_path),
             "/sys/class/net/%s/statistics/tx_bytes", iface);
    snprintf(rx_path, sizeof(rx_path),
             "/sys/class/net/%s/statistics/rx_bytes", iface);

    /* Convert Mbps to bytes/sec: Mbps * 1000000 / 8 */
    nic_speed_bps = nic_speed_mbps * 1000000L / 8;

    /* Read initial counters */
    prev_tx = read_sysfs_ull(tx_path);
    prev_rx = read_sysfs_ull(rx_path);

    return 0;
}

/*
 * netp_sample -- Read current counters and compute utilization
 *
 * Returns: Network physical utilization as percentage (0.0 - 100.0)
 */
double netp_sample(void)
{
    /* TODO: Read current tx_bytes and rx_bytes */
    /* TODO: Compute delta from previous sample */
    /* TODO: Normalize to NIC speed */
    /* TODO: Store current as previous for next sample */
    /* TODO: Return utilization percentage */
    return 0.0;
}
