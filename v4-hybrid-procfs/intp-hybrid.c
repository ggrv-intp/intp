/*
 * intp-hybrid.c -- IntP Hybrid Interference Profiler (V4)
 *
 * Main program for the hybrid procfs/perf_event/resctrl IntP variant.
 * Collects all 7 interference metrics using only stable kernel interfaces:
 *   - netp:  /sys/class/net/<iface>/statistics/ (sysfs)
 *   - nets:  /proc/net/dev (procfs, polling approximation)
 *   - blk:   /proc/diskstats (procfs)
 *   - mbw:   /sys/fs/resctrl/mon_data/.../mbm_total_bytes (resctrl)
 *   - llcmr: perf_event_open() with PERF_TYPE_HW_CACHE (syscall)
 *   - llcocc: /sys/fs/resctrl/mon_groups/.../llc_occupancy (resctrl)
 *   - cpu:   /proc/stat (procfs)
 *
 * No kernel module, no debuginfo, no framework dependency.
 *
 * Usage: sudo ./intp-hybrid -p <PID> -i <interval_ms> [-o csv|text]
 *
 * Equivalent to: sudo stap -g intp.stp <PID> <interval_ms>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

/* Forward declarations for per-metric modules */
/* TODO: Create a shared header (intp.h) with these declarations */

/* netp.c */
extern int netp_init(const char *iface, long nic_speed_mbps);
extern double netp_sample(void);

/* nets.c */
extern int nets_init(const char *iface);
extern double nets_sample(void);

/* blk.c */
extern int blk_init(const char *device);
extern double blk_sample(void);

/* mbw.c */
extern int mbw_init(long max_bw_mbps);
extern double mbw_sample(void);

/* llcmr.c */
extern int llcmr_init(pid_t target_pid);
extern double llcmr_sample(void);
extern void llcmr_cleanup(void);

/* llcocc.c */
extern int llcocc_init(pid_t target_pid, long llc_size_kb);
extern double llcocc_sample(void);

/* cpu.c */
extern int cpu_init(pid_t target_pid);
extern double cpu_sample(void);

/* detect.c */
extern long detect_nic_speed(const char *iface);
extern long detect_llc_size_kb(void);
extern long detect_mem_bw_mbps(void);
extern const char *detect_default_iface(void);

static volatile sig_atomic_t running = 1;

static void sighandler(int sig)
{
    (void)sig;
    running = 0;
}

static void usage(const char *prog)
{
    fprintf(stderr, "Usage: %s -p <PID> -i <interval_ms> [options]\n", prog);
    fprintf(stderr, "\nOptions:\n");
    fprintf(stderr, "  -p, --pid <PID>          Target process ID\n");
    fprintf(stderr, "  -i, --interval <ms>      Sampling interval in ms\n");
    fprintf(stderr, "  -o, --output <format>    Output format: text (default), csv\n");
    fprintf(stderr, "  -n, --nic <iface>        Network interface (auto-detected)\n");
    fprintf(stderr, "  -d, --disk <device>      Block device (auto-detected)\n");
    fprintf(stderr, "  -h, --help               Show this help\n");
}

int main(int argc, char *argv[])
{
    /* TODO: Implement argument parsing */
    pid_t target_pid = 0;
    int interval_ms = 1000;
    const char *output_format = "text";
    const char *nic_iface = NULL;
    const char *disk_device = NULL;

    static struct option long_opts[] = {
        {"pid",      required_argument, NULL, 'p'},
        {"interval", required_argument, NULL, 'i'},
        {"output",   required_argument, NULL, 'o'},
        {"nic",      required_argument, NULL, 'n'},
        {"disk",     required_argument, NULL, 'd'},
        {"help",     no_argument,       NULL, 'h'},
        {NULL, 0, NULL, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "p:i:o:n:d:h", long_opts, NULL)) != -1) {
        switch (opt) {
        case 'p':
            target_pid = (pid_t)atoi(optarg);
            break;
        case 'i':
            interval_ms = atoi(optarg);
            break;
        case 'o':
            output_format = optarg;
            break;
        case 'n':
            nic_iface = optarg;
            break;
        case 'd':
            disk_device = optarg;
            break;
        case 'h':
        default:
            usage(argv[0]);
            return (opt == 'h') ? 0 : 1;
        }
    }

    if (target_pid <= 0) {
        fprintf(stderr, "Error: valid PID required (-p)\n");
        usage(argv[0]);
        return 1;
    }

    /* TODO: Auto-detect hardware parameters */
    if (nic_iface == NULL)
        nic_iface = detect_default_iface();

    long nic_speed = detect_nic_speed(nic_iface);
    long llc_size = detect_llc_size_kb();
    long mem_bw = detect_mem_bw_mbps();

    /* TODO: Initialize all metric collectors */
    /* netp_init(nic_iface, nic_speed); */
    /* nets_init(nic_iface); */
    /* blk_init(disk_device); */
    /* mbw_init(mem_bw); */
    /* llcmr_init(target_pid); */
    /* llcocc_init(target_pid, llc_size); */
    /* cpu_init(target_pid); */

    (void)nic_speed;
    (void)llc_size;
    (void)mem_bw;
    (void)output_format;
    (void)disk_device;

    signal(SIGINT, sighandler);
    signal(SIGTERM, sighandler);

    /* TODO: Print header */
    printf("# IntP Hybrid Profiler -- PID %d, interval %d ms\n",
           target_pid, interval_ms);
    printf("# netp  nets  blk  mbw  llcmr  llcocc  cpu\n");

    /* TODO: Main polling loop */
    struct timespec sleep_ts = {
        .tv_sec  = interval_ms / 1000,
        .tv_nsec = (interval_ms % 1000) * 1000000L
    };

    while (running) {
        /* TODO: Sample all metrics */
        /* double v_netp   = netp_sample(); */
        /* double v_nets   = nets_sample(); */
        /* double v_blk    = blk_sample(); */
        /* double v_mbw    = mbw_sample(); */
        /* double v_llcmr  = llcmr_sample(); */
        /* double v_llcocc = llcocc_sample(); */
        /* double v_cpu    = cpu_sample(); */

        /* TODO: Output in IntP-compatible format */
        /* printf("%.2f  %.2f  %.2f  %.2f  %.2f  %.2f  %.2f\n",
               v_netp, v_nets, v_blk, v_mbw, v_llcmr, v_llcocc, v_cpu); */

        nanosleep(&sleep_ts, NULL);
    }

    /* TODO: Cleanup */
    /* llcmr_cleanup(); */

    printf("# IntP Hybrid Profiler stopped\n");
    return 0;
}
