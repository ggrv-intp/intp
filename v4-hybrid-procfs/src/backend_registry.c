/*
 * backend_registry.c -- target binding and metric_t lifecycle helpers.
 *
 * Holds the singleton intp_target_t and provides metric_select_backend(),
 * metric_init(), metric_read(), metric_cleanup() used by main.
 */

#include "backend.h"
#include "intp.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static intp_target_t g_target;

void intp_target_set(const intp_target_t *t)
{
    if (t) g_target = *t;
}

const intp_target_t *intp_target_get(void)
{
    return &g_target;
}

int metric_select_backend(metric_t *m)
{
    if (!m) return -1;
    if (m->n_backends == 0) return -1;
    for (int i = 0; i < m->n_backends; i++) {
        backend_t *b = m->backends[i];
        if (!b || !b->probe) continue;
        if (b->probe() == 0) {
            m->active = b;
            return 0;
        }
    }
    m->active = NULL;
    return -1;
}

int metric_init(metric_t *m)
{
    if (!m || !m->active || !m->active->init) return -1;
    if (m->active->init() != 0) {
        if (m->active->cleanup) m->active->cleanup();
        m->active = NULL;
        return -1;
    }
    return 0;
}

void metric_read(metric_t *m, metric_sample_t *out, double interval_sec)
{
    if (!out) return;
    out->value      = NAN;
    out->status     = METRIC_STATUS_UNAVAILABLE;
    out->backend_id = "none";
    out->note       = NULL;

    if (!m || !m->active || !m->active->read) return;
    if (m->active->read(out, interval_sec) != 0) {
        out->status     = METRIC_STATUS_UNAVAILABLE;
        out->backend_id = m->active->backend_id;
    }
}

void metric_cleanup(metric_t *m)
{
    if (!m) return;
    for (int i = 0; i < m->n_backends; i++) {
        backend_t *b = m->backends[i];
        if (b && b->cleanup) b->cleanup();
    }
    m->active = NULL;
}

int metric_force_backend(metric_t *m, const char *backend_id)
{
    if (!m || !backend_id) return -1;
    for (int i = 0; i < m->n_backends; i++) {
        backend_t *b = m->backends[i];
        if (b && b->backend_id && strcmp(b->backend_id, backend_id) == 0) {
            if (!b->probe || b->probe() == 0) {
                m->active = b;
                return 0;
            }
            return -1;
        }
    }
    return -1;
}

void metric_disable(metric_t *m)
{
    if (m) m->active = NULL;
}

metric_t **intp_all_metrics(int *n_out)
{
    static metric_t *all[7];
    all[0] = metric_netp();
    all[1] = metric_nets();
    all[2] = metric_blk();
    all[3] = metric_mbw();
    all[4] = metric_llcmr();
    all[5] = metric_llcocc();
    all[6] = metric_cpu();
    if (n_out) *n_out = 7;
    return all;
}
