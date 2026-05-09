/* SPDX-License-Identifier: MIT */
/*
 * probe.c — runtime capability probes.
 *
 * These probes report whether the kernel and the calling process can
 * exercise a given backend without loading any persistent BPF program
 * or attaching anywhere. They exist so packagers, install tests, and
 * monitoring agents can answer "will intp work on this host?" before
 * the sampling loop is committed.
 */

#include <errno.h>

#include <intp/intp.h>
#include <intp/version.h>

#if INTP_HAVE_EBPF
#include <bpf/libbpf.h>
#include <linux/bpf.h>
#endif

__attribute__((visibility("default")))
enum intp_probe_result intp_probe_ebpf(void)
{
#if INTP_HAVE_EBPF
	/* Silence libbpf's stderr chatter; the caller translates the
	 * result code into user-facing text. */
	libbpf_set_print(NULL);

	/* KPROBE is the lowest-common-denominator BPF program type. The
	 * full per-metric backend list probes its own type in Stage 2. */
	int rc = libbpf_probe_bpf_prog_type(BPF_PROG_TYPE_KPROBE, NULL);
	if (rc > 0)
		return INTP_PROBE_OK;
	if (rc == 0)
		return INTP_PROBE_KERNEL_TOO_OLD;
	if (rc == -EPERM || rc == -EACCES)
		return INTP_PROBE_NOT_PERMITTED;
	return INTP_PROBE_ERROR;
#else
	return INTP_PROBE_UNSUPPORTED;
#endif
}
