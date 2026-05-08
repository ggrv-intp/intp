# Contributing to `intp`

Thanks for considering a contribution. `intp` is a small, focused
utility — the surface area is intentionally tight, and that is what
lets it ship as an `apt install`-able tool. Almost everything beyond
the seven core metrics belongs in a [plugin](docs/PLUGINS.md), not in
the core.

This document is the practical "how to land a change" guide. The
*why* lives in [docs/VISION.md](docs/VISION.md); the *what's next*
lives in [docs/ROADMAP.md](docs/ROADMAP.md); the *how it's built*
lives in [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).

---

## Quick start

```bash
# 1. Get the build deps (Debian/Ubuntu)
sudo apt install -y meson ninja-build clang pkg-config \
                    libbpf-dev bpftool linux-tools-generic \
                    cppcheck shellcheck clang-format

# 2. Configure & build
meson setup build
meson compile -C build

# 3. Run the test suite
meson test -C build

# 4. Try the binary
./build/intp --version
./build/intp --help
./build/intp --print-schema
```

For Fedora, Arch, Alpine, and Nix instructions, see
[docs/BUILDING.md](docs/BUILDING.md).

---

## Where to put your change

| You want to…                              | It goes in…                          |
|-------------------------------------------|--------------------------------------|
| Fix a bug in one of the 7 core metrics    | `src/backends/<kind>/`               |
| Add a new collection mechanism for a core metric | `src/backends/<kind>/` + ADR  |
| Add a *new* metric (GPU, DB, vector, app) | `src/plugins/<name>/` (new plugin)   |
| Add a new output format                   | `src/output/<format>.c`              |
| Write a Kubernetes / SLURM / IADA adapter | `integrations/<target>/`             |
| Improve docs                              | `docs/` (one file, one topic)        |
| Improve packaging                         | `packaging/<distro>/`                |
| Add a language binding                    | `bindings/<lang>/`                   |

If you're not sure, open a draft PR or a discussion before writing
much code. Re-routing a misplaced contribution is more work than
asking up front.

---

## Commit & PR conventions

- **Commit messages** follow [Conventional Commits](https://www.conventionalcommits.org/):
  `feat(backend/ebpf): add CO-RE relocation for ring-buffer reader`,
  `fix(output/jsonl): emit schema_version on every line`,
  `docs(plugins): clarify ABI guarantees`.
- **DCO sign-off** is required on every commit (`git commit -s`). We
  do **not** use a CLA — sign-off matches kernel/Debian practice and
  is what downstream packagers expect.
- **One logical change per PR.** A PR that "fixes the build *and*
  refactors the registry *and* adds a metric" gets bounced. Split it.
- **PR description must include:**
  - What the change does and why (the *why* is what reviewers care about).
  - How you tested it. For metric changes: which workload pair, which
    kernel, which CPU. For build/packaging changes: which distros.
  - Any breaking changes flagged at the top, with migration notes.
- **Conventional review labels:** `area/backend`, `area/plugin-abi`,
  `area/packaging-debian`, `kind/bug`, `kind/feat`, `kind/docs`,
  `breaking`, `needs-rebase`. Maintainers add these; you don't need to.

---

## Code style

- **C** — kernel-style, 8-column tabs, `clang-format` enforced. The
  config is in [`.clang-format`](.clang-format); CI fails on
  unformatted code. Run `clang-format -i $(git diff --name-only \
  --diff-filter=AM HEAD~1 -- '*.c' '*.h')` before pushing.
- **Shell** — POSIX-clean where reasonable; bash-isms allowed in
  `tools/` only. `shellcheck`-clean is required.
- **Python** (orchestrators, converters) — `ruff` for lint and format;
  3.10+; type hints on public functions.
- **No new external dependencies in the core** without an ADR. Every
  added `Build-Depends` / `Requires` makes the package harder to land
  on a new distro.

---

## Tests

Three tiers, all run by `meson test`:

- **Unit** (`tests/unit/`) — one test per backend; fixture-driven from
  canned `/proc`/`/sys` trees in `tests/fixtures/`. Fast, hermetic, no
  privileges.
- **Integration** (`tests/integration/`) — end-to-end CLI runs against
  fixture trees. Verifies the output schema and exit codes.
- **Plugin ABI** (`tests/plugin-abi/`) — loads the
  `examples/plugin-hello-world/` plugin and asserts it registers,
  publishes a sample, and appears in `intp --print-schema`. Every
  ABI-touching PR runs `abidiff` against the previous tag.

Live-kernel tests (eBPF load, real `perf_event_open`, real `resctrl`)
live in `tests/integration/live/` and are gated behind
`meson test --suite live` because they need root and a recent kernel.
CI runs them only on the eBPF-enabled job rows.

A new metric or backend is **not done** until it has at least one
fixture-driven unit test and one cross-variant equivalence assertion
in `tests/regression/`.

---

## Reviews

- Two maintainer approvals to merge into `main`. ABI-touching changes
  require a `area/plugin-abi` reviewer (see [CODEOWNERS](.github/CODEOWNERS)).
- Reviews focus on correctness, scope, and packaging impact in that
  order. Style is the linter's job.
- "I'd do it differently" is not a blocker; "this breaks downstream"
  is. State which kind your comment is.

---

## Reporting bugs

Open an issue using the `bug` template. The template asks for:

- `intp --version` output and the exact CLI you ran.
- Kernel version (`uname -r`) and distribution.
- CPU vendor & model (`/proc/cpuinfo` first stanza).
- Whether eBPF is in use (`intp --print-schema` shows the active
  backend per metric).
- Reproducer if possible — a minimal `stress-ng` invocation is ideal.

For metric-fidelity regressions specifically, use the
`metric-fidelity-regression` template; it asks for the comparison
against `intp-comparison`'s reference output.

For security issues, see [SECURITY.md](SECURITY.md) — please do not
open a public issue for kernel-adjacent vulnerabilities.

---

## Plugin authors

Writing an out-of-tree plugin? Read [docs/PLUGINS.md](docs/PLUGINS.md)
first. The plugin ABI is versioned and (post-1.0) stable. The
`tools/intp-plugin-skel/` scaffolder generates a buildable starter
project.

We welcome plugins under any OSI-approved license. Plugins do **not**
need to live in this repository — that's the point of the ABI. If you
want yours listed in the official `docs/PLUGINS.md` registry, open a
PR adding the entry; we ask only that the plugin be open-source and
that the SPDX identifier in its descriptor matches the actual license.

---

## Licensing

By contributing, you agree that your contribution is licensed under
the [MIT license](LICENSE) (the project license). The DCO sign-off on
each commit is your acknowledgement of this.

---

## Communication

- **GitHub Discussions** for design questions, plugin proposals, and
  "is this a bug or expected?" conversations.
- **GitHub Issues** for bugs, packaging breakage, and kernel-compat
  reports.
- **`#intp` on the PUCRS GGRV channels** for the project's internal
  research crowd; outside contributors don't need to be there.
