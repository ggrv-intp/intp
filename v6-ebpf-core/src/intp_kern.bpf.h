/*
 * intp_kern.bpf.h -- Shared Header for IntP eBPF Programs (V6)
 *
 * Defines data structures shared between kernel-space eBPF programs
 * (intp_kern.bpf.c) and the userspace loader (main.c). These structs
 * are used to pass metric data through the BPF ring buffer.
 *
 * Also defines BPF map declarations used by the eBPF programs.
 */

#ifndef __INTP_KERN_BPF_H__
#define __INTP_KERN_BPF_H__

/* Event types for the ring buffer */
enum intp_event_type {
    INTP_EVENT_NETP     = 0,  /* Network physical: byte count */
    INTP_EVENT_NETS_TX  = 1,  /* Network stack TX: service time ns */
    INTP_EVENT_NETS_RX  = 2,  /* Network stack RX: service time ns */
    INTP_EVENT_BLK      = 3,  /* Block I/O: service time ns */
    INTP_EVENT_CPU      = 4,  /* CPU: on-cpu time ns */
    INTP_EVENT_LLCMR    = 5,  /* LLC: miss/ref counts (periodic) */
};

/*
 * intp_event -- Ring buffer event structure
 *
 * Sent from eBPF programs to userspace for each recorded event.
 * The userspace aggregator accumulates these over the sampling interval.
 */
struct intp_event {
    unsigned int  type;       /* enum intp_event_type */
    unsigned int  pid;        /* Process ID */
    unsigned long long timestamp_ns;  /* Event timestamp (ktime) */
    unsigned long long value;         /* Metric-specific value */

    /* Additional fields for specific metrics */
    union {
        struct {
            unsigned long long tx_bytes;
            unsigned long long rx_bytes;
        } netp;

        struct {
            unsigned long long service_ns;
        } nets;

        struct {
            unsigned long long service_ns;
            unsigned int       nr_sectors;
        } blk;

        struct {
            unsigned long long on_cpu_ns;
        } cpu;

        struct {
            unsigned long long cache_refs;
            unsigned long long cache_misses;
        } llcmr;
    };
};

/*
 * Ring buffer size: 256 KB
 * At 1M events/sec with 64-byte events, this gives ~4ms of buffering.
 * Adjust if events are dropped (check ring buffer stats).
 */
#define INTP_RINGBUF_SIZE  (256 * 1024)

/*
 * Maximum number of CPUs to track (for per-CPU maps)
 */
#define INTP_MAX_CPUS  256

/*
 * Maximum entries in hash maps (for tracking in-flight requests)
 */
#define INTP_MAX_ENTRIES  10240

#endif /* __INTP_KERN_BPF_H__ */
