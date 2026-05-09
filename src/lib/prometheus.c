/* SPDX-License-Identifier: MIT */
/*
 * prometheus.c — Prometheus exposition HTTP server.
 *
 * Serves /metrics in the canonical Prometheus text format. In the
 * placeholder phase the seven core metrics report value 0 with
 * status="placeholder", and intp_metrics_implemented advertises 0
 * so scraping operators can see at a glance that the binary is
 * reachable but not yet collecting. Stage 2 wires the live sampling
 * loop into the same endpoint with no API change.
 *
 * Text format only. The protobuf-based remote_write path is rare in
 * practice and would drag libprotobuf into libintp.so for everyone;
 * if it's ever needed, it belongs in a separate optional sink.
 */

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <intp/intp.h>
#include <intp/metrics.h>
#include <intp/version.h>

#if INTP_HAVE_PROMETHEUS

#include <microhttpd.h>

static volatile sig_atomic_t prom_stop;

static void prom_signal_handler(int sig)
{
	(void)sig;
	prom_stop = 1;
}

/* Render the placeholder /metrics body. Returns a malloc'd buffer that
 * MHD will free via MHD_RESPMEM_MUST_FREE; sets *out_len to its size.
 * Returns NULL on allocation or formatting failure. */
static char *prom_render_metrics(size_t *out_len)
{
	size_t cap = 4096;
	char *buf = malloc(cap);
	if (!buf)
		return NULL;
	size_t n = 0;

#define APPEND(fmt, ...) do { \
	int w = snprintf(buf + n, cap - n, (fmt), ##__VA_ARGS__); \
	if (w < 0 || (size_t)w >= cap - n) { \
		free(buf); \
		return NULL; \
	} \
	n += (size_t)w; \
} while (0)

	APPEND(
		"# HELP intp_build_info Build-time configuration of the running intp binary.\n"
		"# TYPE intp_build_info gauge\n"
		"intp_build_info{version=\"%s\",ebpf=\"%s\",prometheus=\"%s\"} 1\n",
		intp_version_string(),
		INTP_HAVE_EBPF ? "enabled" : "disabled",
		INTP_HAVE_PROMETHEUS ? "enabled" : "disabled");

	APPEND(
		"# HELP intp_metrics_implemented Core metrics reporting real values; 0 in placeholder builds, 7 once Stage 2 lands.\n"
		"# TYPE intp_metrics_implemented gauge\n"
		"intp_metrics_implemented 0\n");

	for (size_t i = 0; i < INTP_CORE_METRICS_COUNT; i++) {
		/* "system.interference.cpu" → "system_interference_cpu"
		 * (Prometheus metric names disallow dots). */
		const char *src = intp_core_metric_names[i];
		char name[128];
		size_t j = 0;
		for (; src[j] && j + 1 < sizeof(name); j++)
			name[j] = (src[j] == '.') ? '_' : src[j];
		name[j] = '\0';

		APPEND(
			"# HELP %s Placeholder; collection lands in Stage 2 of the intp roadmap.\n"
			"# TYPE %s gauge\n"
			"%s{status=\"placeholder\"} 0\n",
			name, name, name);
	}

#undef APPEND
	*out_len = n;
	return buf;
}

static enum MHD_Result
prom_handler(void *cls, struct MHD_Connection *conn,
	     const char *url, const char *method,
	     const char *version, const char *upload_data,
	     size_t *upload_data_size, void **req_cls)
{
	(void)cls; (void)version; (void)upload_data; (void)upload_data_size;
	(void)req_cls;

	if (strcmp(method, "GET") != 0) {
		const char *body = "Method Not Allowed\n";
		struct MHD_Response *r = MHD_create_response_from_buffer(
			strlen(body), (void *)(uintptr_t)body, MHD_RESPMEM_PERSISTENT);
		enum MHD_Result rc = MHD_queue_response(conn, 405, r);
		MHD_destroy_response(r);
		return rc;
	}

	if (strcmp(url, "/metrics") != 0) {
		const char *body =
			"intp Prometheus exposition server\n"
			"Endpoints:\n"
			"  /metrics — Prometheus text format\n";
		struct MHD_Response *r = MHD_create_response_from_buffer(
			strlen(body), (void *)(uintptr_t)body, MHD_RESPMEM_PERSISTENT);
		enum MHD_Result rc = MHD_queue_response(conn, MHD_HTTP_OK, r);
		MHD_destroy_response(r);
		return rc;
	}

	size_t body_len = 0;
	char *body = prom_render_metrics(&body_len);
	if (!body)
		return MHD_NO;

	struct MHD_Response *r = MHD_create_response_from_buffer(
		body_len, body, MHD_RESPMEM_MUST_FREE);
	MHD_add_response_header(r, "Content-Type",
		"text/plain; version=0.0.4; charset=utf-8");
	enum MHD_Result rc = MHD_queue_response(conn, MHD_HTTP_OK, r);
	MHD_destroy_response(r);
	return rc;
}

__attribute__((visibility("default")))
int intp_serve_prometheus(uint16_t port)
{
	struct MHD_Daemon *d = MHD_start_daemon(
		MHD_USE_INTERNAL_POLLING_THREAD,
		port, NULL, NULL, &prom_handler, NULL,
		MHD_OPTION_END);
	if (!d)
		return -EADDRINUSE;

	/* Restore the caller's signal disposition on exit so libintp
	 * doesn't permanently override their handlers. */
	struct sigaction old_int, old_term;
	struct sigaction sa = { .sa_handler = prom_signal_handler };
	sigemptyset(&sa.sa_mask);
	sigaction(SIGINT, &sa, &old_int);
	sigaction(SIGTERM, &sa, &old_term);

	while (!prom_stop)
		pause();

	sigaction(SIGINT, &old_int, NULL);
	sigaction(SIGTERM, &old_term, NULL);
	MHD_stop_daemon(d);
	return 0;
}

#else  /* !INTP_HAVE_PROMETHEUS */

__attribute__((visibility("default")))
int intp_serve_prometheus(uint16_t port)
{
	(void)port;
	return -ENOSYS;
}

#endif
