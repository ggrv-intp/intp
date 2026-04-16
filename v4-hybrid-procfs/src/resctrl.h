#ifndef INTP_RESCTRL_H
#define INTP_RESCTRL_H

#include <sys/types.h>

#define RESCTRL_PATH_MAX       384
#define RESCTRL_MAX_DOMAINS    16

/* Creates (or reuses) /sys/fs/resctrl/mon_groups/intp_v4_<pid> and assigns pid
 * to its tasks file. Returns 0 on success, -1 if resctrl is unavailable. */
int resctrl_setup(pid_t pid);

/* Returns the monitoring group directory, or NULL if setup failed. */
const char *resctrl_group_dir(void);

/* Finds every mon_L3_* domain inside the group that exposes `filename`
 * (e.g. "mbm_total_bytes" or "llc_occupancy"), writing full paths into
 * `paths`. Returns number of domains found. */
int resctrl_enumerate_domains(const char *filename,
                              char paths[][RESCTRL_PATH_MAX],
                              int max_domains);

/* Sum the counter values at each path. Missing files contribute 0. */
unsigned long long resctrl_sum_counters(char paths[][RESCTRL_PATH_MAX],
                                        int n_paths);

#endif /* INTP_RESCTRL_H */
