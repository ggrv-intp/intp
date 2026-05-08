<p align="center">
  <img src="images/intp.png" alt="intP — Linux interference profiler" width="280">
</p>

# Roadmap — From placeholder to official Linux package

> *"Every Linux package you take for granted was, at some point,
> someone's `git clone && make`. The road from one to the other is
> long, well-trodden, and worth walking carefully."*

This document traces the path from the current state — a public
placeholder repository — to the long-term goal of `intp` being
available as `sudo apt install intp` on mainstream Linux
distributions.

It is written in stages. Each stage has a **gate** (what must be true
before moving on), an **artefact** (what gets produced), and an
honest note on what could **stall** it. The "if all goes well" in the
README applies to every single stage.

---

## Stage 0 — Public name reservation

> **Status: current stage.**

**Gate:** the GitHub repository
[`ggrv-intp/intp`](https://github.com/ggrv-intp/intp) exists, is
public, and links unambiguously to the source-of-truth research
repository
[`ggrv-intp/intp-comparison`](https://github.com/ggrv-intp/intp-comparison).

**Artefact:** this README and these documents.

**What could stall it:** nothing technical. The risk here is purely
naming — namespace conflicts on Launchpad, Debian, and Ubuntu must be
checked before Stage 3 begins.

---

## Stage 1 — Variant selection

**Gate:** the seven variants in `intp-comparison` have been
benchmarked head-to-head on the dissertation's reference hardware,
with the results published in the corresponding chapter.

**Artefact:** a single chosen variant — almost certainly v2 (pure C
on stable kernel ABIs) or v3 (eBPF / CO-RE with libbpf). Both are
kernel-module-free; both are upstream-friendly. The choice will hinge
on:

- **Portability** across kernel versions and CPU vendors (Intel, AMD,
  ARM server).
- **Operational cost** — compile-time toolchain, runtime memory
  footprint, deployment friction.
- **Fidelity** — how close the metric values track the v0 baseline
  from the original *IntP* paper.

**What could stall it:** an unexpected fidelity gap that forces a
hybrid (v2 + selective eBPF probes), which would push the convergence
date back. The
[VARIANT-COMPARISON](https://github.com/ggrv-intp/intp-comparison/blob/main/docs/VARIANT-COMPARISON.md)
document tracks the live state of this analysis.

---

## Stage 2 — Code migration and stabilization

**Gate:** Stage 1 complete, with a written justification for the
choice.

**Artefact:** the chosen variant's code lives at the root of *this*
repository, with:

- A versioned, documented CLI surface (`intp --help`, `intp(8)`
  manpage, machine-readable output formats).
- A minimum-viable test suite, including at least one
  workload-pair-based interference smoke test.
- A `CHANGELOG.md` and a semver discipline (`0.x` while iterating,
  `1.0` once the CLI surface is frozen).
- An explicit `MAINTAINERS` file, since downstream packagers care
  about who they will be talking to in five years.

**What could stall it:** scope creep. The temptation at this stage is
to keep adding metrics or backends; the antidote is a frozen scope
matching the dissertation's evaluated configuration.

---

## Stage 3 — Debian packaging

**Gate:** Stage 2 produced a tagged release (`v1.0.0` or later) that
builds reproducibly from a clean checkout on a vanilla Debian sid
chroot.

**Artefact:** a `debian/` directory under this repository (or in a
`debian/` branch following the `git-buildpackage` convention), with:

- `debian/control` with proper `Build-Depends` and `Depends`.
- `debian/copyright` in the machine-readable DEP-5 format, listing
  the MIT license and any vendored dependencies.
- `debian/rules` using `dh` with the right buildsystem.
- `debian/changelog` starting at `1.0.0-1`.
- An installable manpage, a default config file (if any), and
  optional `systemd` units for long-running profiler modes.
- A passing `lintian` run with no errors and minimal warnings.

**What could stall it:** vendored dependencies that Debian policy
forbids. eBPF/libbpf has historically been one of those — the
solution is to depend on the system `libbpf-dev` rather than vendor
it. This is one reason variant selection in Stage 1 matters.

**Reference:**
[Debian New Maintainer's Guide](https://www.debian.org/doc/manuals/maint-guide/),
[Debian Policy Manual](https://www.debian.org/doc/debian-policy/).

---

## Stage 4 — Personal PPA on Launchpad

**Gate:** Stage 3 produces a `.deb` that installs cleanly on Ubuntu
LTS releases (currently 22.04, 24.04, and 26.04 once released).

**Artefact:** a personal PPA at
`ppa:<owner>/intp` on [Launchpad](https://launchpad.net), serving
signed `.deb` packages for active LTS series, built from the same
`debian/` directory.

**Why this stage matters:** it is the first moment when an external
user can run `sudo apt install intp` for real. Friction here exposes
issues — missing dependencies, packaging quirks, manpage formatting
— that are cheap to fix on a personal PPA and expensive to fix
later.

**What could stall it:** Launchpad build failures on architectures
the development machine does not exercise (typically `arm64` and
`riscv64`). The mitigation is to test cross-arch builds via
`pbuilder` or `sbuild` before uploading.

**Reference:**
[Launchpad PPA documentation](https://help.launchpad.net/Packaging/PPA).

---

## Stage 5 — Debian sponsorship

**Gate:** Stage 4 has been live long enough to surface real-world
packaging issues, and the package has been used by at least a small
number of external users on the PPA.

**Artefact:** a Debian
[Intent To Package (ITP)](https://www.debian.org/devel/wnpp/) bug
filed against `wnpp`, followed by a sponsored upload to Debian
unstable via [`mentors.debian.net`](https://mentors.debian.net).

**Process:**

1. File an ITP bug describing the package and its license.
2. Upload the package to `mentors.debian.net` for community review.
3. Find a Debian Developer (DD) willing to sponsor the upload —
   typically by participating in the relevant `debian-mentors` mailing
   list discussions and addressing review feedback.
4. After sponsored upload, the package enters the NEW queue, where
   the FTP team reviews license and packaging hygiene.
5. Once accepted, the package is in Debian unstable and starts its
   migration to testing.

**What could stall it:** the NEW queue review can take weeks to
months. Quality issues surface here that did not on a personal PPA;
each round of feedback is a re-upload.

**Reference:**
[Debian Mentors FAQ](https://wiki.debian.org/DebianMentorsFaq),
[Debian NEW queue](https://ftp-master.debian.org/new.html).

---

## Stage 6 — Ubuntu universe (automatic)

**Gate:** Stage 5 succeeded; the package is in Debian testing.

**Artefact:** the package appears in Ubuntu's `universe` component
through the
[Debian-to-Ubuntu auto-sync process](https://wiki.ubuntu.com/SyncRequestProcess),
typically within one Ubuntu development cycle of landing in Debian.

**Why this is "automatic":** Ubuntu pulls source packages from Debian
unstable and rebuilds them. As long as the Debian package builds in
Ubuntu's environment, no separate work is required. As long as.

**What could stall it:** version skew between Debian unstable and
Ubuntu's `main` build dependencies — most often around `libbpf`,
`clang`, or kernel headers. Mitigation: keep the Debian build clean
on Ubuntu's testing environment via PPA tests in Stage 4.

---

## Stage 7 — Adjacent distributions

**Gate:** the package has been stable in Debian and Ubuntu for at
least one release cycle.

**Artefacts:**

- **Fedora:** start with a [COPR](https://copr.fedorainfracloud.org)
  build, then submit a Fedora package review for inclusion in the
  official repositories. The Fedora packaging guidelines are stricter
  about explicit `BuildRequires` and *systemd* integration than
  Debian.
- **Arch Linux:** start in the [AUR](https://aur.archlinux.org). The
  jump from AUR to `community` (now folded into `extra`) requires a
  Trusted User to adopt the package.
- **Alpine Linux:** submit an `aports` package; Alpine uses musl, so
  any non-portable libc assumption surfaces here first.
- **Nix / nixpkgs:** add a derivation to `nixpkgs`. Nix's
  hermetic-build philosophy aligns well with the kernel-module-free
  design, but it tends to expose any non-deterministic build steps.

**What could stall it:** each distribution has its own packaging
culture, review cadence, and policy quirks. The goal is breadth, not
universal coverage; if Fedora and Arch are reachable, that already
covers a substantial fraction of Linux users.

---

## Stage 8 — Kernel-side maturity

This is the "nice to have" stage and is not a precondition for any of
the others.

The longer-term ambition is for the eBPF-based variant (or its
descendants) to track kernel evolution closely enough that:

- New `perf` events relevant to interference (e.g. AMD's PQoS, ARM's
  MPAM) are picked up automatically through CO-RE.
- Hardware-vendor support converges enough that `mbw` and `llcocc`
  no longer require Intel-RDT-specific code paths.
- The tool's output stabilises into a format that downstream
  schedulers (Kubernetes, Slurm, OpenStack) can consume directly,
  rather than re-parsing.

This stage has no fixed deadline. It is what keeps the project alive
beyond the dissertation defense.

---

## Cross-cutting concerns

A few things matter at every stage and are worth listing once:

### Reproducibility

Every release tag in this repository must be buildable from a clean
checkout in five years. Build steps, dependencies, and test data
should be archived alongside the code, not assumed to remain
available externally.

### Citation hygiene

The dissertation defense (expected March 2027) will produce the
canonical academic reference. Until then, the
[`CITATION.cff`](https://github.com/ggrv-intp/intp-comparison/blob/main/CITATION.cff)
in `intp-comparison` is the source of truth for citing this work.

### Naming and namespace

The string `intp` is short and intuitive but also collision-prone.
Before Stage 4, a sweep of namespaces will be needed: Debian
(`apt-cache search intp`), Ubuntu, Launchpad, AUR, COPR, nixpkgs,
PyPI, crates.io, npm, GitHub. The fallback name is `intp-profiler`,
held in reserve.

### Honest scope

Each stage adds new scope only where it serves the goal of an
installable, maintainable utility. Features that exist only in the
research repository (the seven-way comparison harness, the historical
SystemTap variants) stay there. The shipped `intp` is the minimum
that delivers the seven metrics reliably.

---

## Tracking progress

Stage transitions will be reflected as releases on the
[GitHub releases page](https://github.com/ggrv-intp/intp/releases)
and as updates to the README's *status* badge. The commit history of
this repository is the public ledger of the journey.

If you are reading this and the README still says **public placeholder**,
we are still on Stage 0. That is fine. Some roads are worth walking
slowly.

---

<p align="center">
  <em>One <code>apt install</code> at a time.</em>
</p>
