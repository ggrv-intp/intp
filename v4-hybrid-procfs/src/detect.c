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
    char path[256];
    snprintf(path, sizeof(path), "/sys/class/net/%s/speed", iface);

    FILE *f = fopen(path, "r");
    if (!f)
        return 1000;

    long mbps = 0;
    if (fscanf(f, "%ld", &mbps) != 1)
        mbps = 0;
    fclose(f);

    return (mbps > 0) ? mbps : 1000;
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
    const char *base = "/sys/devices/system/cpu/cpu0/cache";
    DIR *d = opendir(base);
    if (!d)
        return 0;

    int  max_level = 0;
    long max_size_kb = 0;

    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
        if (strncmp(entry->d_name, "index", 5) != 0)
            continue;

        char path[320];
        snprintf(path, sizeof(path), "%s/%.63s/level", base, entry->d_name);

        FILE *f = fopen(path, "r");
        if (!f)
            continue;
        int level = 0;
        if (fscanf(f, "%d", &level) != 1) {
            fclose(f);
            continue;
        }
        fclose(f);

        if (level <= max_level)
            continue;

        snprintf(path, sizeof(path), "%s/%.63s/size", base, entry->d_name);
        f = fopen(path, "r");
        if (!f)
            continue;
        char raw[32] = {0};
        if (fscanf(f, "%31s", raw) != 1) {
            fclose(f);
            continue;
        }
        fclose(f);

        char *end;
        long n = strtol(raw, &end, 10);
        if (n <= 0)
            continue;
        if (*end == 'M' || *end == 'm')
            n *= 1024;
        else if (*end != 'K' && *end != 'k' && *end != '\0')
            continue;

        max_level = level;
        max_size_kb = n;
    }
    closedir(d);

    return max_size_kb;
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
    static char iface[64] = "eth0";
    char fallback[64] = {0};

    DIR *net_dir = opendir("/sys/class/net/");
    if (!net_dir)
        return iface;

    struct dirent *entry;
    while ((entry = readdir(net_dir)) != NULL) {
        if (entry->d_name[0] == '.')
            continue;
        if (strcmp(entry->d_name, "lo") == 0)
            continue;

        if (fallback[0] == '\0')
            snprintf(fallback, sizeof(fallback), "%.63s", entry->d_name);

        char path[320];
        snprintf(path, sizeof(path),
                 "/sys/class/net/%.63s/operstate", entry->d_name);

        FILE *f = fopen(path, "r");
        if (!f)
            continue;

        char state[32] = {0};
        if (fscanf(f, "%31s", state) == 1 && strcmp(state, "up") == 0) {
            fclose(f);
            snprintf(iface, sizeof(iface), "%.63s", entry->d_name);
            closedir(net_dir);
            return iface;
        }
        fclose(f);
    }
    closedir(net_dir);

    if (fallback[0] != '\0')
        snprintf(iface, sizeof(iface), "%s", fallback);
    return iface;
}
