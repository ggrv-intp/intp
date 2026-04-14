/*
 * detect.c -- Hardware Detection Functions (V4 hybrid)
 *
 * Provides hardware detection for normalizing IntP metrics:
 *   - NIC speed (for netp normalization)
 *   - LLC size (for llcocc normalization)
 *   - Memory bandwidth (for mbw normalization)
 *   - Default network interface
 *   - RDT/PQoS capability flags
 *
 * These functions read from sysfs, procfs, and optionally dmidecode.
 * They are the C equivalent of shared/intp-detect.sh.
 *
 * All detection functions return sensible defaults when detection fails,
 * so the profiler can still run with approximate normalization.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

/*
 * detect_nic_speed -- Get NIC link speed from sysfs
 *
 * Reads /sys/class/net/<iface>/speed (value in Mbps).
 *
 * Parameters:
 *   iface - Network interface name (e.g., "eth0")
 *
 * Returns: NIC speed in Mbps, or 1000 (1 Gbps) as default
 */
long detect_nic_speed(const char *iface)
{
    /* TODO: Read /sys/class/net/<iface>/speed */
    /* TODO: Return value if valid (> 0), else return 1000 */
    (void)iface;
    return 1000;
}

/*
 * detect_llc_size_kb -- Get LLC size from cache topology sysfs
 *
 * Iterates /sys/devices/system/cpu/cpu0/cache/index* directories,
 * finds the highest level cache, and reads its size.
 *
 * Returns: LLC size in KB, or 0 if detection fails
 */
long detect_llc_size_kb(void)
{
    /* TODO: Iterate /sys/devices/system/cpu/cpu0/cache/index* */
    /* TODO: For each, read "level" file, find max */
    /* TODO: Read "size" file of the max-level cache */
    /* TODO: Parse "XXXK" or "XXXM" format */
    return 0;
}

/*
 * detect_mem_bw_mbps -- Estimate maximum memory bandwidth
 *
 * Attempts to determine maximum memory bandwidth from:
 * 1. dmidecode --type 17 (requires root)
 * 2. Known defaults by CPU model
 * 3. Conservative default (DDR4-2666 dual-channel = 42656 MB/s)
 *
 * Returns: Estimated max memory bandwidth in MB/s
 */
long detect_mem_bw_mbps(void)
{
    /* TODO: Try dmidecode if available and running as root */
    /* TODO: Parse speed and data width from DIMM info */
    /* TODO: Estimate channels from number of populated DIMMs */
    /* TODO: BW = speed * width_bytes * channels */

    /* Default: DDR4-2666 dual-channel */
    /* 2666 MT/s * 8 bytes * 2 channels = 42656 MB/s */
    return 42656;
}

/*
 * detect_default_iface -- Find the default network interface
 *
 * Strategy: first non-loopback interface that is UP.
 * Fallback: first non-loopback interface regardless of state.
 *
 * Returns: Static string with interface name, or "eth0" as fallback
 */
const char *detect_default_iface(void)
{
    /* TODO: Iterate /sys/class/net/ */
    /* TODO: Skip "lo" */
    /* TODO: Check operstate == "up" */
    /* TODO: Return first match */
    static char iface[64] = "eth0";

    /* TODO: Implement detection */
    (void)iface;
    return iface;
}
