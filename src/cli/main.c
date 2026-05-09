/* SPDX-License-Identifier: MIT */
/*
 * intp(1) — Linux interference profiler CLI.
 *
 * Stage 1.5 scaffold: this main.c handles --version, --help, and a
 * static --print-schema. The real argument parsing, libintp wiring,
 * and output-sink dispatch land in Stage 2 of the roadmap, when the
 * backend implementations are ready to back them.
 *
 * The CLI is intentionally a thin wrapper over libintp; everything
 * the binary can do, a linked program can also do via <intp/intp.h>.
 */

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <intp/intp.h>

static const char usage_text[] =
"Usage: intp [OPTIONS]\n"
"\n"
"Linux interference profiler — measures cross-workload interference\n"
"across CPU, last-level cache, memory bandwidth, block I/O, and\n"
"network resources. Produces structured samples on stdout.\n"
"\n"
"Targeting:\n"
"  -p, --pid PID              profile a specific process\n"
"  -c, --cgroup PATH          profile a cgroup-v2 path (post-1.0)\n"
"      (default: profile the whole system)\n"
"\n"
"Sampling:\n"
"  -i, --interval DURATION    sampling interval (default: 1s; e.g. 100ms)\n"
"  -d, --duration DURATION    stop after DURATION (default: run forever)\n"
"\n"
"Output:\n"
"  -f, --format FORMAT        jsonl (default), tsv-legacy-v1, prometheus, otlp\n"
"      --print-schema         dump active schema as JSON Schema and exit\n"
"      --serve[=URI]          long-running exposition mode (Prometheus on :9100)\n"
"                             URI examples: prometheus  prometheus://:9090\n"
"                             (note: must use --serve=URI, not --serve URI)\n"
"\n"
"Diagnostics:\n"
"      --probe                check whether eBPF backends will work here\n"
"\n"
"Plugins:\n"
"      --plugin-dir DIR       additional plugin search directory\n"
"      --enable-plugin NAME   force a plugin to load (comma-separated)\n"
"      --disable-plugin NAME  force a plugin off (comma-separated)\n"
"      --disable-backend NAME force a core backend off\n"
"\n"
"Misc:\n"
"  -v, --verbose              increase logging level\n"
"  -V, --version              print version and build options, then exit\n"
"  -h, --help                 print this help and exit\n"
"\n"
"For documentation see intp(1) or https://github.com/ggrv-intp/intp.\n";

static void print_version(void)
{
	printf("intp %s\n", intp_version_string());
	printf("Build options:\n");
#if INTP_HAVE_EBPF
	printf("  eBPF backends:        enabled\n");
#else
	printf("  eBPF backends:        disabled\n");
#endif
#if INTP_HAVE_BPFTRACE
	printf("  bpftrace plugin:      enabled\n");
#endif
#if INTP_HAVE_GPU_NVML
	printf("  GPU NVML plugin:      enabled\n");
#endif
#if INTP_HAVE_GPU_ROCM
	printf("  GPU ROCm plugin:      enabled\n");
#endif
#if INTP_HAVE_GPU_ZES
	printf("  GPU Level Zero plug.: enabled\n");
#endif
#if INTP_HAVE_PROMETHEUS
	printf("  Prometheus sink:      enabled\n");
#else
	printf("  Prometheus sink:      disabled\n");
#endif
#if INTP_HAVE_OTEL
	printf("  OpenTelemetry sink:   enabled\n");
#endif
	printf("\n");
	printf("intp is the production successor to intp-comparison.\n");
	printf("License: MIT. See LICENSE in the source distribution.\n");
}

static void print_help(void)
{
	fputs(usage_text, stdout);
}

static void print_schema_static(void)
{
	/*
	 * Stage 1.5: emit a static representation of the *advertised*
	 * schema. The real intp_print_schema() (which reflects the
	 * active backend selection on the running host) lands in
	 * Stage 2.
	 */
	printf("{\n");
	printf("  \"$schema\": \"https://json-schema.org/draft/2020-12/schema\",\n");
	printf("  \"intp_version\": \"%s\",\n", intp_version_string());
	printf("  \"schema_version\": \"0.1.0-dev\",\n");
	printf("  \"status\": \"stage-1.5-scaffold\",\n");
	printf("  \"core_metrics\": [\n");
	for (size_t i = 0; i < INTP_CORE_METRICS_COUNT; i++) {
		printf("    \"%s\"%s\n",
		       intp_core_metric_names[i],
		       (i + 1 < INTP_CORE_METRICS_COUNT) ? "," : "");
	}
	printf("  ],\n");
	printf("  \"note\": \"Real schema reflecting active backends lands in Stage 2.\"\n");
	printf("}\n");
}

enum {
	OPT_PRINT_SCHEMA = 1000,
	OPT_PLUGIN_DIR,
	OPT_ENABLE_PLUGIN,
	OPT_DISABLE_PLUGIN,
	OPT_DISABLE_BACKEND,
	OPT_SERVE,
	OPT_PROBE,
};

static const struct option long_opts[] = {
	{ "pid",            required_argument, NULL, 'p' },
	{ "cgroup",         required_argument, NULL, 'c' },
	{ "interval",       required_argument, NULL, 'i' },
	{ "duration",       required_argument, NULL, 'd' },
	{ "format",         required_argument, NULL, 'f' },
	{ "verbose",        no_argument,       NULL, 'v' },
	{ "version",        no_argument,       NULL, 'V' },
	{ "help",           no_argument,       NULL, 'h' },
	{ "print-schema",   no_argument,       NULL, OPT_PRINT_SCHEMA },
	{ "plugin-dir",     required_argument, NULL, OPT_PLUGIN_DIR },
	{ "enable-plugin",  required_argument, NULL, OPT_ENABLE_PLUGIN },
	{ "disable-plugin", required_argument, NULL, OPT_DISABLE_PLUGIN },
	{ "disable-backend", required_argument, NULL, OPT_DISABLE_BACKEND },
	{ "serve",          optional_argument, NULL, OPT_SERVE },
	{ "probe",          no_argument,       NULL, OPT_PROBE },
	{ NULL, 0, NULL, 0 },
};

/* Run intp_probe_ebpf() and write a one-line human-readable status.
 * Exit code semantics let scripts branch on the failure mode:
 *   0  ok                      eBPF will work
 *   2  kernel too old          host needs an upgrade
 *   3  not permitted           process needs CAP_BPF / CAP_SYS_ADMIN
 *   4  unsupported             libintp built without eBPF
 *   5  error                   probe itself failed (see errno) */
static int run_probe_ebpf(void)
{
	enum intp_probe_result r = intp_probe_ebpf();
	switch (r) {
	case INTP_PROBE_OK:
		printf("intp probe: eBPF backends OK\n");
		return 0;
	case INTP_PROBE_KERNEL_TOO_OLD:
		printf("intp probe: eBPF UNAVAILABLE — kernel lacks BPF program support (need 5.8+)\n");
		return 2;
	case INTP_PROBE_NOT_PERMITTED:
		printf("intp probe: eBPF UNAVAILABLE — caller lacks CAP_BPF or CAP_SYS_ADMIN\n");
		return 3;
	case INTP_PROBE_UNSUPPORTED:
		printf("intp probe: eBPF UNAVAILABLE — this build was compiled without libbpf\n");
		return 4;
	case INTP_PROBE_ERROR:
	default:
		printf("intp probe: eBPF UNKNOWN — probe failed (errno %d: %s)\n",
		       errno, strerror(errno));
		return 5;
	}
}

/* Parse `--serve [arg]` argument, where arg is one of:
 *   (empty)                       → prometheus on 9100
 *   "prometheus"                  → prometheus on 9100
 *   "prometheus://[host:]port"    → prometheus on the given port
 *   anything else                 → not yet implemented (returns 2)
 *
 * Host part is currently ignored; libmicrohttpd binds 0.0.0.0. Stage 2
 * will introduce an explicit address-binding option. */
static int run_serve(const char *arg)
{
	const char *proto = arg ? arg : "prometheus";
	uint16_t port = 9100;

	if (strncmp(proto, "prometheus", 10) != 0) {
		fprintf(stderr,
			"intp: --serve %s is not yet implemented in this build.\n"
			"      Supported: --serve prometheus[://[host:]port] (default :9100)\n",
			proto);
		return 2;
	}

	const char *rest = proto + 10;
	if (*rest == ':' && rest[1] == '/' && rest[2] == '/') {
		rest += 3;
		const char *colon = strrchr(rest, ':');
		if (colon) {
			long p = strtol(colon + 1, NULL, 10);
			if (p > 0 && p <= 65535)
				port = (uint16_t)p;
		}
	} else if (*rest != '\0') {
		fprintf(stderr, "intp: malformed --serve argument: %s\n", proto);
		return 2;
	}

	fprintf(stderr,
		"intp: serving Prometheus exposition on :%u (Stage 1.5 placeholder values).\n"
		"intp: scrape http://localhost:%u/metrics ; SIGINT to stop.\n",
		port, port);

	int rc = intp_serve_prometheus(port);
	if (rc == -ENOSYS) {
		fprintf(stderr, "intp: --serve prometheus unavailable: built with -Dwith_prometheus=false.\n");
		return 4;
	}
	if (rc < 0) {
		fprintf(stderr, "intp: prometheus server failed: %s\n", strerror(-rc));
		return 1;
	}
	return 0;
}

int main(int argc, char *argv[])
{
	int opt;

	while ((opt = getopt_long(argc, argv, "p:c:i:d:f:vVh",
				  long_opts, NULL)) != -1) {
		switch (opt) {
		case 'V':
			print_version();
			return 0;
		case 'h':
			print_help();
			return 0;
		case OPT_PRINT_SCHEMA:
			print_schema_static();
			return 0;
		case OPT_PROBE:
			return run_probe_ebpf();
		case OPT_SERVE:
			return run_serve(optarg);
		case 'p':
		case 'c':
		case 'i':
		case 'd':
		case 'f':
		case 'v':
		case OPT_PLUGIN_DIR:
		case OPT_ENABLE_PLUGIN:
		case OPT_DISABLE_PLUGIN:
		case OPT_DISABLE_BACKEND:
			fprintf(stderr,
				"intp: option not yet implemented in this scaffold build.\n"
				"      Stage 2 of the roadmap will wire it up.\n"
				"      Run `intp --version`, `intp --print-schema`, `intp --probe`,\n"
				"      or `intp --serve` for now.\n");
			return 2;
		default:
			fprintf(stderr, "Try 'intp --help' for usage.\n");
			return 2;
		}
	}

	/* No options: print a friendly message instead of just exiting silently. */
	fprintf(stderr,
		"intp %s — scaffold build (Stage 1.5).\n"
		"Real sampling lands in Stage 2; for now try:\n"
		"    intp --version\n"
		"    intp --help\n"
		"    intp --print-schema\n"
		"    intp --probe\n"
		"    intp --serve\n",
		intp_version_string());
	return 0;
}
