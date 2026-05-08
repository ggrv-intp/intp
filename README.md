# intp

<img src="docs/images/intp.png" alt="intP — Linux interference profiler" width="720">

**Interference profiling. Precise observability. For Linux.**

<p align="center">
  <img alt="status" src="https://img.shields.io/badge/status-public%20placeholder-yellow?style=flat-square">
  <img alt="destination" src="https://img.shields.io/badge/destination-PPA%20%E2%86%92%20official%20package-success?style=flat-square">
  <img alt="kernel" src="https://img.shields.io/badge/Linux-kernel%206.8%2B-informational?style=flat-square">
  <img alt="license" src="https://img.shields.io/badge/license-MIT-blue?style=flat-square">
</p>

---

> **This repository is a name reservation.** A public marker for the place
> where the **`intp`** utility will land once it is ready to be packaged and
> — **if all goes well** — submitted as an official Linux package
> (PPA on Launchpad → Debian → Ubuntu → other distributions).
>
> The code that will live here is not born in this directory. It is
> designed, compared, and hardened in
> **[intp-comparison](https://github.com/ggrv-intp/intp-comparison)**,
> where **seven parallel variants** compete, side by side, to become the
> implementation that earns the `intp` name on `apt install`.

## The vision in one sentence

To measure, precisely and cheaply, **how one workload interferes with
another** when they share the same Linux hardware — CPU, last-level
cache, memory bandwidth, block I/O, and network. In production. Without
assembling a zoo of tools.

```bash
# the end goal
sudo apt install intp
intp -p $(pgrep -f my-application) -i 100ms
```

## Seven metrics. One single utility.

`intp` is the direct heir of the original *interference profiler*
published by Xavier & De Rose (SBAC-PAD 2022, PUCRS), rewritten to
survive modern kernels and contemporary clouds. In short windows it
collects seven signals that answer the question *"who is stealing what
from whom?"*:

| Metric    | What it measures                                   | Source |
|-----------|----------------------------------------------------|--------|
| **netp**  | NIC physical utilization (TX + RX in bps)          | driver counters |
| **nets**  | Kernel network stack service time                  | softirq / NAPI |
| **blk**   | Block device busy percentage                       | block layer |
| **mbw**   | Memory bandwidth (LLC ↔ DRAM)                      | uncore IMC PMU |
| **llcmr** | LLC miss ratio                                     | perf_event |
| **llcocc**| LLC occupancy in bytes                             | Intel RDT / resctrl |
| **cpu**   | CPU utilization (user + sys)                       | scheduler / cgroup |

Each metric is a window; the set is the *fingerprint* of interference.
The technical deep-dive lives in
[METRICS-DEEP-DIVE](https://github.com/ggrv-intp/intp-comparison/blob/main/docs/METRICS-DEEP-DIVE.md).

## Why this exists

Every cloud — public or private — sells the illusion that vCPUs are
independent. They are not. Co-located workloads fight over cache, over
memory bandwidth, over NIC queues, over I/O queues. The symptom is
latency that appears with no obvious cause; the cause is the *noisy
neighbour* nobody can see.

Traditional tools (`top`, `sar`, `iostat`, `perf`) show *what each
thing consumes*. `intp` shows **how that consumption translates into
observable interference** — the signal that interference-aware
schedulers (such as
[IADA](https://www.sciencedirect.com/science/article/abs/pii/S0164121222001670))
need in order to decide.

`intp` is not a replacement for `htop`. It is the missing sensor
between *"this metric is high"* and *"this is hurting the neighbour"*.

## Relationship with `intp-comparison`

The sibling repository
[`intp-comparison`](https://github.com/ggrv-intp/intp-comparison) is the
test bench. Seven implementations coexist there, each with a different
combination of instrumentation paradigm, cost, portability, and
fidelity:

| Variant       | Paradigm                                  | Kernel module | Min. kernel |
|---------------|-------------------------------------------|:-------------:|:-----------:|
| v0            | Classic SystemTap (original paper)        | yes           | ≤ 6.6       |
| v0.1          | SystemTap, patched for 6.8+               | yes           | 6.8+        |
| v1            | SystemTap native probes                   | yes           | 6.8+        |
| v1.1          | SystemTap + userspace helper              | yes           | 6.8+        |
| **v2**        | **Pure C: procfs / perf_event / resctrl** | **no**        | **4.10+**   |
| **v3**        | **eBPF / CO-RE with libbpf**              | **no**        | **5.8+**    |
| v3.1          | bpftrace + Python orchestrator            | no            | 5.8+        |

Only the **kernel-module-free** variants (v2 and v3) survive the
upstream packaging bar — official distributions do not accept
out-of-tree modules without a long-term maintenance path. When the
experimental comparison is done, the winning variant lands here and
becomes `intp`.

> The research behind that decision is the Master's dissertation of
> **André Sacilotto Santos**, advised by **Prof. Cesar De Rose**
> (PPGCC/PUCRS), with defense expected in **March 2027**.

## Roadmap to the official package

The journey from "GitHub placeholder" to `sudo apt install intp` has
well-defined stops. The full breakdown lives in
[docs/ROADMAP.md](docs/ROADMAP.md); at a glance:

1. **Public name reservation** — *you are here.*
2. **Variant selection** — `intp-comparison` converges on a single
   kernel-module-free implementation.
3. **Debian packaging** — `debian/` directory, `dh_make`, manpages,
   optional systemd unit, reproducible install tests.
4. **Personal PPA on Launchpad** — first public channel for Ubuntu LTS
   (22.04, 24.04, 26.04).
5. **Debian sponsorship** — *Intent To Package* (ITP),
   `mentors.debian.net` review, sponsored upload by a Debian Developer.
6. **Ubuntu universe** — automatic sync from Debian.
7. **Adjacent distributions** — Fedora (COPR → official), Arch (AUR →
   official), Alpine, Nix.

Every step has its own technical gate (quality, tests, distro policy)
and its own bureaucratic gate. None of them is trivial. That is why
the *"if all goes well"* is not false modesty — it is honesty about a
process that involves more than code.

## How to follow along

Until the package is born, the place to watch the work happening is
[`intp-comparison`](https://github.com/ggrv-intp/intp-comparison) —
home of the variants, the benchmarks, and the technical documents
([METRICS-DEEP-DIVE](https://github.com/ggrv-intp/intp-comparison/blob/main/docs/METRICS-DEEP-DIVE.md),
[KERNEL-6.8-CHANGES](https://github.com/ggrv-intp/intp-comparison/blob/main/docs/KERNEL-6.8-CHANGES.md),
[VARIANT-COMPARISON](https://github.com/ggrv-intp/intp-comparison/blob/main/docs/VARIANT-COMPARISON.md)).

Local documentation:

- [docs/VISION.md](docs/VISION.md) — *Why `intp`, and why now.*
- [docs/ROADMAP.md](docs/ROADMAP.md) — *The steps to the official package.*

## Citation

Until the defense happens, please cite the source repository
(`intp-comparison`) using the metadata in
[CITATION.cff](https://github.com/ggrv-intp/intp-comparison/blob/main/CITATION.cff).
The dissertation reference will be added after the defense, expected
in **March 2027**.

Original work:

> Xavier, M. G. and De Rose, C. A. F. (2022).
> *IntP: Quantifying Cross-Application Interference via System-Level Instrumentation*.
> SBAC-PAD 2022, Bordeaux, France, pp. 221–230. IEEE. PUCRS.
> [PDF](https://repositorio.pucrs.br/dspace/bitstream/10923/24018/2/IntP_Quantifying_crossapplication_interference_via_systemlevel_instrumentation.pdf) ·
> [IEEE](https://ieeexplore.ieee.org/document/9980934/)

## License

[MIT](LICENSE).

---

<p align="center">
  <em>The name is reserved. The code is on its way.</em>
</p>
