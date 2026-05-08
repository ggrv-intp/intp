# Security policy

`intp` reads from kernel interfaces, opens `perf_event` file descriptors,
and (when built with eBPF) loads programs that the kernel verifier
runs in privileged context. Bugs in this code can have outsized
consequences. We treat security reports accordingly.

---

## Reporting a vulnerability

**Please do not open a public issue for security problems.** Instead,
use one of the private channels below.

### Preferred: GitHub Security Advisories

Open a private advisory at
<https://github.com/ggrv-intp/intp/security/advisories/new>. This is
encrypted, audited, and lets us coordinate a fix and disclosure
timeline before the report becomes public.

### Email

If you cannot use GitHub Security Advisories, email the maintainer
listed in [MAINTAINERS](MAINTAINERS) directly with
`[intp-security]` in the subject line. PGP welcome but not required;
include a short summary in the body and attach details as needed.

### What to include

- A description of the issue and the affected version(s) (`intp
  --version`).
- Reproduction steps. A minimal proof-of-concept is ideal.
- Kernel version (`uname -r`), distribution, CPU, and whether the
  eBPF backend was active.
- Your assessment of impact (DoS, info-leak, privilege escalation,
  metric falsification, etc.).
- Whether you intend to publish independently and on what timeline,
  so we can align disclosure.

We aim to acknowledge reports within **3 business days** and to land
a fix or a documented mitigation within **90 days** of acknowledgement,
sooner for high-severity issues. Reporters are credited in the advisory
and the changelog unless they ask to remain anonymous.

---

## Scope

In scope:

- The `intp` binary, `libintp`, and any in-tree plugin shipped from
  this repository.
- The plugin loader (`src/core/`) — including how it validates plugin
  descriptors and handles malicious or malformed `.so` files.
- The eBPF programs in `src/backends/ebpf/` and the verifier interaction.
- The streaming subscriber API (`src/ipc/`) and its authentication
  model.
- The Debian / RPM / Arch packaging in `packaging/` — packaging bugs
  with security impact (e.g. world-writable directories, dangerous
  default config) are in scope.
- Out-of-tree plugins distributed via official `intp-plugin-*` packages
  by the project maintainers.

Out of scope (please report upstream):

- Bugs in `libbpf`, the kernel, `libelf`, `bpftool`, or other
  dependencies.
- Bugs in third-party plugins not shipped by this project.
- Issues that require a malicious local administrator who already has
  `CAP_SYS_ADMIN` or `CAP_BPF` — those capabilities trivially defeat
  any user-space defence; we document this in
  [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).

---

## Threat model summary

Full discussion lives in `docs/ARCHITECTURE.md` once written. In brief:

- `intp` is a *measurement* tool, not a security boundary. Its job is
  to produce honest numbers; protecting the host is the kernel's job.
- We assume the operator running `intp` has the capabilities they
  declared they have (`CAP_PERFMON`, `CAP_BPF`, `CAP_DAC_READ_SEARCH`,
  etc.). The plugin descriptor mechanism is a *clarity* tool, not a
  *containment* tool — a malicious plugin loaded by a privileged
  caller has the privileges of that caller.
- Where we do harden: input parsing of `/proc` and `/sys` text files
  (no fixed-size buffer assumptions), bounds checking on all eBPF
  ring-buffer reads, refusing to load plugins with a mismatched
  `INTP_PLUGIN_ABI_VERSION`, and rejecting plugin descriptors whose
  declared capabilities exceed the running process's actual capabilities.

---

## Disclosure timeline

For coordinated disclosure on validated vulnerabilities:

1. **T+0:** Report received and acknowledged privately.
2. **T+0–14 days:** Triage, severity assessment, reproduction.
3. **T+14–60 days:** Fix developed and reviewed privately. CVE
   requested if appropriate.
4. **T+60–90 days:** Coordinated public disclosure: GitHub advisory
   published, fix released, distro packagers notified via the standard
   `linux-distros` mailing list when warranted.

We can move faster on critical severity, and can hold longer if the
reporter requests it for responsible-disclosure reasons.
