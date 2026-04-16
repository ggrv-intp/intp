/*
 * blk.c -- Block I/O Utilization (V4 hybrid)
 *
 * Reads field 13 (io_ticks) from /proc/diskstats. delta_io_ticks / delta_ms
 * is the same "%util" iostat prints. Capped at 100% even though NVMe multi-
 * queue devices can legitimately exceed it -- we preserve [0,100] semantics
 * across variants.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "intp.h"

static char                 disk_name[64];
static unsigned long long   prev_io_ticks;
static struct timespec      prev_ts;
static int                  ok;

static int parse_diskstats(const char *device, unsigned long long *io_ticks)
{
    FILE *f = fopen("/proc/diskstats", "r");
    if (!f)
        return -1;

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        unsigned int maj, min;
        char name[64];
        unsigned long long f4, f5, f6, f7, f8, f9, f10, f11, f12, io_tk;
        int matched = sscanf(line,
            "%u %u %63s %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
            &maj, &min, name,
            &f4, &f5, &f6, &f7, &f8, &f9, &f10, &f11, &f12, &io_tk);
        if (matched != 13)
            continue;
        if (strcmp(name, device) == 0) {
            *io_ticks = io_tk;
            fclose(f);
            return 0;
        }
    }
    fclose(f);
    return -1;
}

static int autodetect_disk(char *out, size_t out_sz)
{
    FILE *f = fopen("/proc/diskstats", "r");
    if (!f)
        return -1;

    char line[512];
    char best_name[64] = {0};
    unsigned long long best_ticks = 0;

    while (fgets(line, sizeof(line), f)) {
        unsigned int maj, min;
        char name[64];
        unsigned long long f4, f5, f6, f7, f8, f9, f10, f11, f12, io_tk;
        int matched = sscanf(line,
            "%u %u %63s %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
            &maj, &min, name,
            &f4, &f5, &f6, &f7, &f8, &f9, &f10, &f11, &f12, &io_tk);
        if (matched != 13)
            continue;
        if (strncmp(name, "loop", 4) == 0) continue;
        if (strncmp(name, "ram",  3) == 0) continue;
        if (strncmp(name, "zram", 4) == 0) continue;
        if (strncmp(name, "dm-",  3) == 0) continue;
        /* skip partitions: sda1, nvme0n1p1 */
        size_t len = strlen(name);
        if (len > 0 && name[len - 1] >= '0' && name[len - 1] <= '9') {
            if (strncmp(name, "nvme", 4) != 0 &&
                strncmp(name, "mmcblk", 6) != 0)
                continue;
            /* nvme0n1p1 / mmcblk0p1 have 'p' before trailing digits */
            char *p = strrchr(name, 'p');
            if (p && p > name && *(p - 1) >= '0' && *(p - 1) <= '9')
                continue;
        }
        if (io_tk >= best_ticks) {
            best_ticks = io_tk;
            snprintf(best_name, sizeof(best_name), "%s", name);
        }
    }
    fclose(f);

    if (best_name[0] == '\0')
        return -1;
    snprintf(out, out_sz, "%s", best_name);
    return 0;
}

int blk_init(const char *device)
{
    if (device != NULL) {
        snprintf(disk_name, sizeof(disk_name), "%s", device);
    } else if (autodetect_disk(disk_name, sizeof(disk_name)) < 0) {
        ok = 0;
        return -1;
    }

    if (parse_diskstats(disk_name, &prev_io_ticks) < 0) {
        ok = 0;
        return -1;
    }
    clock_gettime(CLOCK_MONOTONIC, &prev_ts);
    ok = 1;
    return 0;
}

double blk_sample(void)
{
    if (!ok)
        return NAN;

    unsigned long long now_ticks = 0;
    if (parse_diskstats(disk_name, &now_ticks) < 0)
        return NAN;

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    double dt_ms = (now.tv_sec  - prev_ts.tv_sec)  * 1000.0
                 + (now.tv_nsec - prev_ts.tv_nsec) / 1e6;
    if (dt_ms <= 0.0)
        return 0.0;

    double util = (double)(now_ticks - prev_io_ticks) / dt_ms * 100.0;

    prev_io_ticks = now_ticks;
    prev_ts       = now;

    if (util < 0.0)   util = 0.0;
    if (util > 100.0) util = 100.0;
    return util;
}

const char *blk_device_name(void)
{
    return disk_name;
}
