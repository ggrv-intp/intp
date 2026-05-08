# Integrations

`intp` produces structured interference data; downstream tools turn
that data into decisions, dashboards, and alerts. This document is
the cookbook for wiring `intp` into the systems most users will reach
for.

For each integration, we cover *what to deploy*, *what it costs*, and
*what it produces*. Code lives under [`integrations/`](../integrations/);
each subdirectory has its own README with deployment specifics.

---

## Prometheus

**Deploy as:** `intp` invoked in serve mode by systemd, scraped by
your existing Prometheus instance.

```ini
# /etc/systemd/system/intp-collector.service
[Unit]
Description=intp interference profiler (Prometheus exposition)
After=network.target

[Service]
ExecStart=/usr/bin/intp serve --prometheus :9100 --interval 5s
Restart=on-failure
AmbientCapabilities=CAP_PERFMON CAP_DAC_READ_SEARCH

[Install]
WantedBy=multi-user.target
```

```yaml
# scrape config
- job_name: 'intp'
  static_configs:
    - targets: ['<node>:9100']
  scrape_interval: 5s
```

**What you get:** the seven core metrics as Prometheus gauges, plus
any plugin-defined metrics. Standard naming: `intp_<metric>_ratio`
for percentages, `intp_<metric>_bytes` for sizes, `intp_info` for
build/host metadata.

**Cost:** ~5 MB resident, < 1% CPU at 5 s cadence. eBPF-enabled mode
is no costlier than disabled mode.

---

## OpenTelemetry

**Deploy as:** `intp` configured to push OTLP to your OTel Collector,
which then fans out to whatever your observability stack already
consumes (Tempo, Loki, Datadog, New Relic, …).

```bash
intp serve --otlp grpc://otel-collector.observability:4317 \
           --otlp-headers "x-tenant=cluster-a" \
           --interval 1s
```

The OTLP semantic-convention mapping (in
`integrations/opentelemetry/`) places the seven core metrics under
`system.interference.*` and plugin-defined metrics under
`system.interference.<plugin>.*`. Resource attributes carry host
identity so multiple `intp` instances don't collide.

**What you get:** correlation. An interference spike measured by
`intp` can be linked to specific traces (Jaeger / Tempo) by trace ID
when your application code propagates one.

**Cost:** OTLP is heavier on the wire than Prometheus; expect ~10×
the bytes per sample compared to Prometheus exposition. Use a longer
interval (1–5 s) and OTLP-over-HTTP if gRPC is awkward.

---

## Kubernetes

Three pieces, deployable independently.

### 1. DaemonSet — per-node `intp` collector

`integrations/k8s/daemonset/` provides a Helm chart and raw YAML.
Deploys `intp` as a DaemonSet with `hostPID: true`, mounts
`/sys/fs/resctrl` and `/proc`, exposes Prometheus on port 9100, and
optionally pushes OTLP.

```bash
helm install intp-collector ./integrations/k8s/daemonset \
     --set otlp.endpoint=otel-collector:4317 \
     --set ebpf.enabled=true
```

The Pod runs with `securityContext.capabilities.add: ["PERFMON",
"BPF", "DAC_READ_SEARCH"]` — *not* `privileged: true`. eBPF requires
the host kernel ≥ 5.8 and BTF.

### 2. Scheduler plugin — interference-aware pod placement

`integrations/k8s/kube-scheduler-plugin/` implements a
[scheduler-plugins](https://github.com/kubernetes-sigs/scheduler-plugins)
extension. The plugin subscribes to `intp` over gRPC, computes a
per-node interference score, and biases the scoring phase against
nodes whose `mbw + llcocc + cpu` composite is above a configurable
threshold.

This is an **opinionated choice** about which signals to weight; the
default is the composite from the IntP paper, but the weights are
exposed as configuration. Operators that want a different scheme can
fork the plugin without touching `intp`.

### 3. Descheduler strategy — rebalance under interference

`integrations/k8s/descheduler-strategy/` adds a `HighInterference`
strategy to [`descheduler`](https://github.com/kubernetes-sigs/descheduler)
that evicts pods from nodes whose interference signature has been
above a threshold for N consecutive evaluations.

### 4. `InterferenceProfile` CRD (read-only)

Deployed alongside the DaemonSet, the CRD exposes the current
interference profile per node as a Kubernetes resource. Custom
controllers (KEDA, autoscalers, alerting hooks) can `kubectl get
interferenceprofiles` instead of scraping Prometheus.

---

## SLURM

**Deploy as:** prolog/epilog hook on each compute node.

`integrations/slurm/intp-prolog.sh` runs `intp` for the duration of
the job, attaches it to the job's cgroup (`/sys/fs/cgroup/slurm/.../`),
writes the per-job profile to `${SLURM_SUBMIT_DIR}/intp.tsv`. The
epilog shuts it down. Existing schedulers can read the resulting file
or pipe `intp serve` output via Unix socket to a SPANK plugin.

`integrations/slurm/spank-intp/` is a SPANK plugin stub for live
integration; it requires building against the local SLURM headers and
is not in v1.0 scope.

---

## IADA

[IADA](https://www.sciencedirect.com/science/article/abs/pii/S0164121222001670)
is the PUCRS interference-aware scheduler that consumes the IntP
profile directly. Two modes:

### Offline (file-based)

Run `intp` for each candidate workload, convert the TSV with
`tools/convert-meyer.py` (ported from
`intp-comparison/bench/convert-profiler-to-meyer.py`), feed the
resulting CSV into IADA. This is bit-for-bit compatible with the
original IADA pipeline.

```bash
intp -p $(pgrep -f my-workload) --interval 1s --duration 600 \
     --format tsv-legacy-v1 > profile.tsv
python3 tools/convert-meyer.py profile.tsv > my-workload.csv
```

### Online (streaming)

`integrations/iada/iada-subscriber.py` subscribes to `intp serve`
over the gRPC streaming API and feeds samples into IADA's runtime
loop. This is post-1.0 scope and depends on IADA having a runtime-
input interface.

---

## Grafana

`integrations/grafana/` ships:

- **Dashboards** — `dashboards/intp-overview.json` (host-level
  interference at a glance), `dashboards/intp-noisy-neighbour.json`
  (per-PID, drills down).
- **Loki/Tempo correlator panel** — a panel JSON that joins `intp`'s
  metric spikes with Loki logs and Tempo spans by host + time
  window. Requires OTel correlation IDs to flow end-to-end.

Import via `Grafana → Dashboards → Import` or by HTTP API in your
provisioning config.

---

## APM (Datadog, New Relic, Dynatrace)

These vendors all consume OTLP. The path is:

1. `intp serve --otlp grpc://otel-collector:4317`
2. OTel Collector → vendor exporter (each vendor has its own).

There is no per-vendor code in `intp` itself; the OTel mapping in
`integrations/opentelemetry/` is what makes this work uniformly.

If a vendor wants tighter integration (e.g. a richer encoding, native
correlation), they can write an `output_sink` plugin against the
public ABI without touching the core. We will list it in
[PLUGINS.md](PLUGINS.md) under *Community plugins*.

---

## Service mesh & tracing (Istio, Linkerd, Jaeger)

Service-mesh metrics correlate naturally with `intp` via OTLP — both
publish into the same Collector, which joins them by trace ID, host,
and time window. There is **no in-tree mesh code**: the integration
is "configure your mesh and `intp` to talk to the same OTel
Collector".

A worked example with Istio + Tempo + the `intp` Helm chart lives in
`integrations/grafana/correlator-example.md`.

---

## Custom integrations

The two extension points to consider, in order:

1. **An `output_sink` plugin** — the cheapest path. Write a `.so`
   against the public ABI; the format becomes available as `--format
   <your-name>`. See [PLUGINS.md](PLUGINS.md).
2. **A subscriber over the gRPC streaming API** — when you need
   *live* data and the consumer doesn't sit in the same process. The
   proto definition is in `integrations/grpc/intp.proto`.

For one-off scripts, you can always parse JSON-Lines from `intp
--format jsonl` directly. It is a stable format from 1.0 onward and
self-describes its schema.
