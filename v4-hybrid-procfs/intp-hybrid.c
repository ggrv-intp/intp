/*
 * intp-hybrid.c -- IntP Hybrid Interference Profiler (V4)
 *
 * Collects all 7 interference metrics using only stable kernel ABIs:
 *   netp    sysfs /sys/class/net/<iface>/statistics/
 *   nets    sysfs packet-rate (polling approximation of V1's service time)
 *   blk     procfs /proc/diskstats field 13 (io_ticks)
 *   mbw     resctrl mbm_total_bytes (Intel RDT / AMD QoS / ARM MPAM)
 *   llcmr   perf_event_open HW_CACHE (references, misses)
 *   llcocc  resctrl llc_occupancy
 *   cpu     procfs /proc/<pid>/stat (utime + stime)
 *
 * Metrics that cannot be collected on the current platform degrade to NaN
 * and are printed as "--" (text) or empty (csv) rather than zero.
 *
 * Usage: sudo ./intp-hybrid -p <PID> -i <interval_ms> [-o text|csv]
 *                           [-n <iface>] [-d <disk>]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <math.h>
#include "src/intp.h"
#include "src/resctrl.h"

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
    fprintf(stderr, "  -m, --max-bw <MB/s>      Override peak memory bandwidth for mbw normalization\n");
    fprintf(stderr, "                           (default: 42656 = DDR4-2666 dual-channel)\n");
    fprintf(stderr, "  -h, --help               Show this help\n");
}

static void print_field(double v, int csv)
{
    if (csv) {
        if (isnan(v)) printf(",");
        else          printf(",%.2f", v);
    } else {
        if (isnan(v)) printf("  --  ");
        else          printf("  %6.2f", v);
    }
}

static void timespec_add_ms(struct timespec *ts, int ms)
{
    ts->tv_sec  += ms / 1000;
    ts->tv_nsec += (ms % 1000) * 1000000L;
    if (ts->tv_nsec >= 1000000000L) {
        ts->tv_sec  += ts->tv_nsec / 1000000000L;
        ts->tv_nsec %= 1000000000L;
    }
}

int main(int argc, char *argv[])
{
    pid_t       target_pid      = 0;
    int         interval_ms     = 1000;
    const char *output_format   = "text";
    const char *nic_iface       = NULL;
    const char *disk_device     = NULL;
    long        max_bw_override = 0;  /* 0 = use detect_mem_bw_mbps() default */

    static struct option long_opts[] = {
        {"pid",      required_argument, NULL, 'p'},
        {"interval", required_argument, NULL, 'i'},
        {"output",   required_argument, NULL, 'o'},
        {"nic",      required_argument, NULL, 'n'},
        {"disk",     required_argument, NULL, 'd'},
        {"max-bw",   required_argument, NULL, 'm'},
        {"help",     no_argument,       NULL, 'h'},
        {NULL, 0, NULL, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "p:i:o:n:d:m:h",
                              long_opts, NULL)) != -1) {
        switch (opt) {
        case 'p': target_pid      = (pid_t)atoi(optarg); break;
        case 'i': interval_ms     = atoi(optarg);        break;
        case 'o': output_format   = optarg;              break;
        case 'n': nic_iface       = optarg;              break;
        case 'd': disk_device     = optarg;              break;
        case 'm': max_bw_override = atol(optarg);        break;
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
    if (interval_ms < 1) {
        fprintf(stderr, "Error: interval must be >= 1 ms\n");
        return 1;
    }
    int csv = (strcmp(output_format, "csv") == 0);

    if (nic_iface == NULL)
        nic_iface = detect_default_iface();

    long nic_speed = detect_nic_speed(nic_iface);
    long llc_size  = detect_llc_size_kb();
    long mem_bw    = (max_bw_override > 0) ? max_bw_override
                                           : detect_mem_bw_mbps();

    fprintf(stderr,
            "[detect] iface=%s nic_speed=%ld Mbps "
            "llc=%ld KB mem_bw=%ld MB/s%s\n",
            nic_iface, nic_speed, llc_size, mem_bw,
            (max_bw_override > 0) ? " (user override)" : "");

    /* resctrl group once; mbw + llcocc share it. Failure is not fatal. */
    int resctrl_ok = (resctrl_setup(target_pid) == 0);

    /* Init metrics. Each returns -1 on unavailable and will yield NaN. */
    int cpu_ok    = (cpu_init(target_pid) == 0);
    int netp_ok   = (netp_init(nic_iface, nic_speed) == 0);
    int nets_ok   = (nets_init(nic_iface) == 0);
    int blk_ok    = (blk_init(disk_device) == 0);
    int mbw_ok    = resctrl_ok && (mbw_init(mem_bw) == 0);
    int llcocc_ok = resctrl_ok && (llcocc_init(target_pid, llc_size) == 0);
    int llcmr_ok  = (llcmr_init(target_pid) == 0);

    fprintf(stderr,
            "[status] cpu=%s netp=%s nets=%s blk=%s(%s) "
            "mbw=%s llcmr=%s llcocc=%s\n",
            cpu_ok    ? "on" : "off",
            netp_ok   ? "on" : "off",
            nets_ok   ? "on" : "off",
            blk_ok    ? "on" : "off",
            blk_ok    ? blk_device_name() : "-",
            mbw_ok    ? "on" : "off",
            llcmr_ok  ? "on" : "off",
            llcocc_ok ? "on" : "off");

    signal(SIGINT, sighandler);
    signal(SIGTERM, sighandler);

    if (csv) {
        printf("time_ms,netp,nets,blk,mbw,llcmr,llcocc,cpu\n");
    } else {
        printf("# IntP Hybrid Profiler (V4) -- PID %d, interval %d ms\n",
               target_pid, interval_ms);
        printf("# time_ms   netp    nets    blk     mbw    llcmr   llcocc    cpu\n");
    }
    fflush(stdout);

    struct timespec start, wake;
    clock_gettime(CLOCK_MONOTONIC, &start);
    wake = start;
    timespec_add_ms(&wake, interval_ms);

    while (running) {
        if (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &wake, NULL) != 0) {
            if (!running) break;
        }
        timespec_add_ms(&wake, interval_ms);

        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        long elapsed_ms = (now.tv_sec  - start.tv_sec)  * 1000L
                        + (now.tv_nsec - start.tv_nsec) / 1000000L;

        /* Check target still exists; exit cleanly if it's gone. */
        char procdir[64];
        snprintf(procdir, sizeof(procdir), "/proc/%d", (int)target_pid);
        if (access(procdir, F_OK) != 0) {
            fprintf(stderr, "[main] target PID %d exited\n", (int)target_pid);
            break;
        }

        double v_netp   = netp_sample();
        double v_nets   = nets_sample();
        double v_blk    = blk_sample();
        double v_mbw    = mbw_sample();
        double v_llcmr  = llcmr_sample();
        double v_llcocc = llcocc_sample();
        double v_cpu    = cpu_sample();

        if (csv) {
            printf("%ld", elapsed_ms);
        } else {
            printf("%9ld", elapsed_ms);
        }
        print_field(v_netp,   csv);
        print_field(v_nets,   csv);
        print_field(v_blk,    csv);
        print_field(v_mbw,    csv);
        print_field(v_llcmr,  csv);
        print_field(v_llcocc, csv);
        print_field(v_cpu,    csv);
        printf("\n");
        fflush(stdout);
    }

    llcmr_cleanup();

    if (!csv)
        fprintf(stderr, "# IntP Hybrid Profiler stopped\n");
    return 0;
}
