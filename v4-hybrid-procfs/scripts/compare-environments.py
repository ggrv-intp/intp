#!/usr/bin/env python3
"""compare-environments.py -- compare V4 captures across bare-metal/container/VM.

Reads the TSV files in the results directory passed as argv[1] and emits a
short Markdown comparison: per-metric mean, median, and the backend-id banner.
"""

import os
import statistics
import sys

METRICS = ["netp", "nets", "blk", "mbw", "llcmr", "llcocc", "cpu"]


def parse_tsv(path):
    """Return (banner_dict, list of per-sample dicts)."""
    banner = {}
    samples = []
    if not os.path.exists(path):
        return banner, samples
    with open(path) as f:
        for line in f:
            line = line.rstrip("\n")
            if line.startswith("# v4 backends:"):
                for token in line.split()[3:]:
                    if "=" in token:
                        k, v = token.split("=", 1)
                        banner[k] = v
            elif line.startswith("netp\t"):
                continue  # column header
            elif line.startswith("#"):
                continue
            else:
                cols = line.split("\t")
                if len(cols) != len(METRICS):
                    continue
                row = {}
                for i, m in enumerate(METRICS):
                    row[m] = None if cols[i] == "--" else float(cols[i])
                samples.append(row)
    return banner, samples


def stats_of(values):
    nums = [v for v in values if v is not None]
    if not nums:
        return None, None
    return statistics.mean(nums), statistics.median(nums)


def main():
    if len(sys.argv) < 2:
        print("Usage: compare-environments.py <results-dir>", file=sys.stderr)
        return 1
    d = sys.argv[1]
    envs = []
    for name in ("baremetal", "container", "vm"):
        path = os.path.join(d, f"{name}.tsv")
        envs.append((name, *parse_tsv(path)))

    print(f"# IntP V4 cross-environment comparison ({d})\n")
    print("## Backends used\n")
    print("| environment | " + " | ".join(METRICS) + " |")
    print("|" + "---|" * (len(METRICS) + 1))
    for name, banner, _ in envs:
        row = [name] + [banner.get(m, "-") for m in METRICS]
        print("| " + " | ".join(row) + " |")

    print("\n## Per-metric mean / median (samples available only)\n")
    for m in METRICS:
        print(f"### {m}\n")
        print("| environment | mean | median | n_samples |")
        print("|---|---|---|---|")
        for name, _, samples in envs:
            mean, med = stats_of([s[m] for s in samples])
            n = sum(1 for s in samples if s[m] is not None)
            mean_s = f"{mean:.2f}" if mean is not None else "n/a"
            med_s  = f"{med:.2f}"  if med  is not None else "n/a"
            print(f"| {name} | {mean_s} | {med_s} | {n} |")
        print()
    return 0


if __name__ == "__main__":
    sys.exit(main())
