/*
 * resctrl.c -- shared resctrl setup for mbw/llcocc (V4 hybrid)
 *
 * resctrl is the cross-vendor interference-monitoring API: Intel RDT (4.10+),
 * AMD QoS (5.1+), ARM MPAM (6.19+). By consolidating setup here, mbw.c and
 * llcocc.c become vendor-neutral file readers.
 *
 * Design:
 *   - One monitoring group per profiler instance: /sys/fs/resctrl/mon_groups/intp_v4_<pid>
 *   - PID is assigned to the group's tasks file
 *   - Domain enumeration returns all mon_L3_* directories (NUMA sockets /
 *     AMD CCX / MPAM partitions). Aggregation is the caller's job.
 *
 * Graceful degradation:
 *   - resctrl_setup() returns -1 if the filesystem is not mounted, if
 *     mon_groups does not exist, or if the group cannot be created. Callers
 *     return NaN from their sample() path, which the main loop prints as "--".
 *   - This is the expected behavior on: consumer CPUs without RDT/QoS,
 *     ARM kernels < 6.19, VMs without passthrough, unprivileged users.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include "intp.h"
#include "resctrl.h"

#define RESCTRL_ROOT "/sys/fs/resctrl"

static char group_dir[256];
static int  group_initialized;

static int path_exists(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0;
}

int resctrl_setup(pid_t pid)
{
    if (group_initialized)
        return 0;

    if (!path_exists(RESCTRL_ROOT "/mon_groups")) {
        fprintf(stderr,
                "[resctrl] %s/mon_groups not present -- "
                "resctrl unmounted or unsupported (AMD/Intel RDT or ARM MPAM needed)\n",
                RESCTRL_ROOT);
        return -1;
    }

    snprintf(group_dir, sizeof(group_dir),
             "%s/mon_groups/intp_v4_%d", RESCTRL_ROOT, (int)pid);

    if (mkdir(group_dir, 0755) != 0 && errno != EEXIST) {
        fprintf(stderr, "[resctrl] mkdir(%s) failed: %s\n",
                group_dir, strerror(errno));
        return -1;
    }

    char tasks_path[320];
    snprintf(tasks_path, sizeof(tasks_path), "%s/tasks", group_dir);
    FILE *f = fopen(tasks_path, "w");
    if (!f) {
        fprintf(stderr, "[resctrl] open %s failed: %s\n",
                tasks_path, strerror(errno));
        return -1;
    }
    fprintf(f, "%d\n", (int)pid);
    fclose(f);

    group_initialized = 1;
    return 0;
}

const char *resctrl_group_dir(void)
{
    return group_initialized ? group_dir : NULL;
}

int resctrl_enumerate_domains(const char *filename,
                              char paths[][RESCTRL_PATH_MAX],
                              int max_domains)
{
    if (!group_initialized)
        return 0;

    char mon_data[260];
    snprintf(mon_data, sizeof(mon_data), "%.250s/mon_data", group_dir);

    DIR *d = opendir(mon_data);
    if (!d)
        return 0;

    int count = 0;
    struct dirent *entry;
    while ((entry = readdir(d)) != NULL && count < max_domains) {
        if (strncmp(entry->d_name, "mon_L3_", 7) != 0)
            continue;

        char candidate[RESCTRL_PATH_MAX];
        snprintf(candidate, sizeof(candidate),
                 "%.250s/%.63s/%.63s", mon_data, entry->d_name, filename);
        if (!path_exists(candidate))
            continue;

        snprintf(paths[count], RESCTRL_PATH_MAX, "%s", candidate);
        count++;
    }
    closedir(d);
    return count;
}

unsigned long long resctrl_sum_counters(char paths[][RESCTRL_PATH_MAX],
                                        int n_paths)
{
    unsigned long long sum = 0;
    for (int i = 0; i < n_paths; i++) {
        FILE *f = fopen(paths[i], "r");
        if (!f)
            continue;
        unsigned long long v = 0;
        if (fscanf(f, "%llu", &v) == 1)
            sum += v;
        fclose(f);
    }
    return sum;
}
