/*
 * nets.c -- Network Stack Utilization (V4 hybrid)
 *
 * IntP metric: nets (network stack utilization / service time)
 * Equivalent to: print_netstack_report() in v1-original/intp.stp
 *
 * Kernel interface:
 *   /proc/net/dev (aggregate RX/TX byte counters per interface)
 *
 * IMPORTANT: This is a POLLING APPROXIMATION of the original metric.
 *
 * The original intp.stp computes network stack service time by timestamping
 * individual packets as they enter and exit the kernel networking stack:
 *   - TX: timestamp at dev_queue_xmit(), measure at net_dev_xmit tracepoint
 *   - RX: timestamp at netif_receive_skb(), measure at ip_rcv()
 *   - Service time = exit_timestamp - entry_timestamp
 *   - Utilization = sum(service_times) / interval * 100
 *
 * Without kernel probes, we CANNOT replicate per-packet latency measurement.
 * Instead, this module reads aggregate byte counters from /proc/net/dev and
 * estimates utilization as throughput relative to a theoretical maximum.
 * This is a fundamentally different measurement -- throughput vs service time.
 *
 * Formula (approximation):
 *   delta_bytes = (rx_now - rx_prev) + (tx_now - tx_prev)
 *   utilization_approx = delta_bytes / (max_throughput * interval) * 100
 *
 * This approximation will:
 *   - Underestimate utilization when packets are small (high PPS, low BPS)
 *   - Miss queue congestion that does not show up in throughput
 *   - Not capture per-packet latency spikes
 *
 * For accurate service-time measurement, use V1/V2/V3 (SystemTap) or
 * V5/V6 (bpftrace/eBPF) which can instrument kernel functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char iface_name[64];
static unsigned long long prev_rx_bytes;
static unsigned long long prev_tx_bytes;

/*
 * parse_proc_net_dev -- Parse /proc/net/dev for a specific interface
 *
 * /proc/net/dev format (after 2 header lines):
 *   iface: rx_bytes rx_packets ... tx_bytes tx_packets ...
 *
 * Returns: 0 on success, -1 if interface not found
 */
static int parse_proc_net_dev(const char *iface,
                              unsigned long long *rx_bytes,
                              unsigned long long *tx_bytes)
{
    /* TODO: Open /proc/net/dev */
    /* TODO: Skip 2 header lines */
    /* TODO: Find line matching iface */
    /* TODO: Parse rx_bytes (field 1) and tx_bytes (field 9) */
    (void)iface;
    (void)rx_bytes;
    (void)tx_bytes;
    return -1;
}

/*
 * nets_init -- Initialize network stack utilization collector
 *
 * Parameters:
 *   iface - Network interface name
 *
 * Returns: 0 on success, -1 on error
 */
int nets_init(const char *iface)
{
    /* TODO: Store interface name and read initial counters */
    strncpy(iface_name, iface, sizeof(iface_name) - 1);
    iface_name[sizeof(iface_name) - 1] = '\0';

    parse_proc_net_dev(iface_name, &prev_rx_bytes, &prev_tx_bytes);
    return 0;
}

/*
 * nets_sample -- Read current counters and compute approximate utilization
 *
 * Returns: Network stack utilization approximation as percentage (0.0 - 100.0)
 *
 * NOTE: This is a throughput-based approximation, NOT a service-time
 * measurement. See file header for details on the difference.
 */
double nets_sample(void)
{
    /* TODO: Read current rx_bytes and tx_bytes from /proc/net/dev */
    /* TODO: Compute delta from previous sample */
    /* TODO: Estimate utilization (throughput-based approximation) */
    /* TODO: Store current as previous for next sample */
    return 0.0;
}
