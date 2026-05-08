# Test fixtures

This directory holds canned input data — `/proc/diskstats` snapshots,
`/proc/stat` samples, `/sys/fs/resctrl/...` trees, recorded eBPF
ring-buffer dumps — used by the unit and integration tests.

The build harness sets `INTP_TEST_FIXTURE_DIR` to this directory's
absolute path (in the source tree), so tests can reference fixtures
by relative path:

```c
FILE *f = intp_test_fixture_open("procfs/diskstats-sapphire-rapids.txt");
```

Conventions:

- One subdirectory per data source: `procfs/`, `sysfs/`,
  `resctrl/`, `perf/`, `ebpf/`.
- File names include enough context (CPU model, kernel version,
  workload) to interpret the fixture without context: e.g.
  `diskstats-stress-ng-vm-1G-kernel6.8.txt`.
- Keep fixtures small. If a real-world capture is large, trim it to
  the relevant sections and document the trim in a header comment.

This directory is intentionally empty in the Stage 1.5 scaffold;
fixtures land alongside the backend implementations in Stage 2.
