#ifndef INTP_H
#define INTP_H

#include <sys/types.h>

#define NIC_BPS_DEFAULT 125000000L

struct intp_sample {
    double netp, nets, blk, mbw, llcmr, llcocc, cpu;
};

/* netp -- network physical utilization (sysfs) */
int    netp_init(const char *iface, long nic_speed_mbps);
double netp_sample(void);

/* nets -- network stack utilization (procfs, polling approximation) */
int    nets_init(const char *iface);
double nets_sample(void);

/* blk -- block I/O utilization (procfs) */
int         blk_init(const char *device);
double      blk_sample(void);
const char *blk_device_name(void);

/* mbw -- memory bandwidth utilization (resctrl) */
int    mbw_init(long max_bw_mbps);
double mbw_sample(void);

/* llcmr -- LLC miss ratio (perf_event_open) */
int    llcmr_init(pid_t target_pid);
double llcmr_sample(void);
void   llcmr_cleanup(void);

/* llcocc -- LLC occupancy (resctrl) */
int    llcocc_init(pid_t target_pid, long llc_size_kb);
double llcocc_sample(void);

/* cpu -- CPU utilization (procfs) */
int    cpu_init(pid_t target_pid);
double cpu_sample(void);

/* Hardware detection (detect.c) */
long        detect_nic_speed(const char *iface);
long        detect_llc_size_kb(void);
long        detect_mem_bw_mbps(void);
const char *detect_default_iface(void);

#endif /* INTP_H */
