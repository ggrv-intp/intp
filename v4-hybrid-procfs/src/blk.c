/*
 * blk.c -- Block I/O Utilization (V4 hybrid)
 *
 * IntP metric: blk (block I/O utilization)
 * Equivalent to: print_block_report() in v1-original/intp.stp
 *
 * Kernel interface:
 *   /proc/diskstats
 *
 * /proc/diskstats format (selected fields):
 *   major minor name ... io_ticks(field 13) ...
 *
 * Field 13 (io_ticks): Number of milliseconds spent doing I/O.
 * This field increments as long as there is at least one I/O request in
 * the queue or being serviced. It is the basis for the "iostat %util" value.
 *
 * Formula:
 *   delta_io_ticks = io_ticks_now - io_ticks_prev
 *   utilization = delta_io_ticks / interval_ms * 100
 *
 * If utilization > 100%, the device has multiple I/O queues (NVMe).
 * Cap at 100% or report raw value depending on configuration.
 *
 * The original intp.stp uses block tracepoints to timestamp individual I/O
 * requests and compute service time. The /proc/diskstats approach gives
 * equivalent utilization numbers (it is what iostat uses) but cannot provide
 * per-request latency distributions.
 *
 * Tradeoffs vs V1:
 *   - Equivalent for utilization percentage
 *   - Cannot provide per-I/O latency (would need blktrace or eBPF)
 *   - Simpler: single file read vs kernel probe instrumentation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char disk_name[64];
static unsigned long long prev_io_ticks;

/*
 * parse_diskstats -- Parse /proc/diskstats for a specific device
 *
 * Returns: 0 on success, -1 if device not found
 */
static int parse_diskstats(const char *device, unsigned long long *io_ticks)
{
    /* TODO: Open /proc/diskstats */
    /* TODO: Find line matching device name */
    /* TODO: Parse field 13 (io_ticks, 0-indexed from name) */
    /* Note: field numbering from start of line: */
    /*   1=major, 2=minor, 3=name, 4=reads_completed, ... 13=io_ticks */
    (void)device;
    (void)io_ticks;
    return -1;
}

/*
 * blk_init -- Initialize block I/O utilization collector
 *
 * Parameters:
 *   device - Block device name (e.g., "sda", "nvme0n1"). NULL for auto-detect.
 *
 * Returns: 0 on success, -1 on error
 */
int blk_init(const char *device)
{
    /* TODO: Auto-detect primary block device if device is NULL */
    /* TODO: Read initial io_ticks value */
    if (device != NULL) {
        strncpy(disk_name, device, sizeof(disk_name) - 1);
        disk_name[sizeof(disk_name) - 1] = '\0';
    } else {
        /* TODO: Auto-detect from /proc/diskstats (pick device with most I/O) */
        strncpy(disk_name, "sda", sizeof(disk_name) - 1);
    }

    parse_diskstats(disk_name, &prev_io_ticks);
    return 0;
}

/*
 * blk_sample -- Read current io_ticks and compute utilization
 *
 * Returns: Block I/O utilization as percentage (0.0 - 100.0)
 */
double blk_sample(void)
{
    /* TODO: Read current io_ticks from /proc/diskstats */
    /* TODO: Compute delta_io_ticks = current - previous */
    /* TODO: utilization = delta_io_ticks / interval_ms * 100 */
    /* TODO: Cap at 100% unless reporting raw values */
    /* TODO: Store current as previous */
    return 0.0;
}
