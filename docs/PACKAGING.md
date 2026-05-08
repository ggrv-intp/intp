# Packaging `intp` for distributions

This document is the playbook for getting `intp` from a tagged release
into a distribution's package repository. It is written from the
perspective of the upstream maintainer. If you are a distro packager
adopting `intp`, this is also the document that should answer your
questions about the project's packaging promises.

The big-picture roadmap (which distro, in what order, against which
gates) lives in [ROADMAP.md](ROADMAP.md). This document is the *how*.

---

## Project promises to packagers

We commit to:

- **Reproducible source releases.** Each tag produces a
  `intp-X.Y.Z.tar.gz` that builds identically from a clean checkout
  on any reasonable build host, for at least five years from the
  release date.
- **Stable build inputs.** No optional dependency surprises within a
  major version: if `1.0.0` builds against `libbpf ≥ 0.8`, every
  `1.x` release also does.
- **Conventional install layout.** `meson install` honours `--prefix`,
  `--libdir`, and the standard FHS paths. No "magic" install steps.
- **DEP-5 / SPDX-correct copyright** in the upstream tarball.
- **Manpages** at `share/man/man1/intp.1` and friends, for `intp`
  itself and any in-tree binary plugins.
- **A versioned `libintp` ABI** post-1.0, with `abidiff` gating in CI.
- **A maintained `MAINTAINERS` file** so packagers know who to ping.

We ask packagers to:

- Use the system `libbpf-dev`, `libmicrohttpd-dev`, etc. rather than
  vendoring; we have no out-of-tree forks of those.
- File any packaging quirks via the
  [distro-packaging issue template](../.github/ISSUE_TEMPLATE/distro-packaging.yml).
- Not ship `intp-plugin-*` packages for plugins we have not blessed in
  [PLUGINS.md](PLUGINS.md) under the same name; pick a vendor-prefixed
  name to avoid namespace collisions.

---

## Artefacts shipped per distro

The base build produces these artefacts (sub-packages where the distro
allows):

| Artefact                | Contents                                                        |
|-------------------------|-----------------------------------------------------------------|
| `intp`                  | The CLI binary and manpage. Depends on `libintp1`.              |
| `libintp1`              | The shared library `libintp.so.1`. Runtime-linkable.            |
| `libintp-dev`           | Public headers, `intp.pc`, static lib (if built).               |
| `intp-doc`              | The `docs/` tree as installed `share/doc/intp/`.                |
| `intp-plugin-bpftrace`  | The bpftrace research plugin.                                   |
| `intp-plugin-gpu-nvml`  | NVIDIA GPU plugin (often a *contrib* / *non-free* repo).        |
| `intp-plugin-gpu-rocm`  | AMD GPU plugin.                                                 |
| `intp-plugin-gpu-zes`   | Intel GPU plugin.                                               |
| `intp-plugin-db-*`      | Database plugins (one sub-package per database).                |

A static-binary release (`intp-static-x86_64.tar.gz`) is attached to
each GitHub release as a fallback for users on distros where `intp`
hasn't landed yet. It is not the recommended distribution path.

---

## Debian (and Ubuntu)

### Source layout

`packaging/debian/` follows standard `debhelper`-13 conventions:

```
packaging/debian/
├── control                # source + binary stanzas
├── changelog              # Debian changelog (separate from upstream CHANGELOG.md)
├── copyright              # DEP-5, lists vendored deps if any
├── rules                  # dh-based, very short
├── source/format          # 3.0 (quilt) or (native), TBD
├── intp.install
├── libintp1.install
├── libintp-dev.install
├── intp-doc.install
├── intp.manpages
├── intp-collector.service # optional systemd unit
├── intp.lintian-overrides # only with explicit justification
└── tests/                 # autopkgtest entries
```

`debian/rules` uses `dh $@ --buildsystem=meson --parallel`. No
patches needed for the upstream tarball — it builds straight.

### Build dependencies

`debian/control` `Build-Depends:` includes:

```
debhelper-compat (= 13),
meson,
ninja-build,
pkg-config,
clang,
libbpf-dev (>= 0.8),
linux-tools-generic | bpftool,
libelf-dev,
zlib1g-dev,
libmicrohttpd-dev,
python3,
```

Optional builds (`gpu-nvml`, `otel`, etc.) get their own
`Build-Depends:` lines under separate `Build-Profiles:` so they don't
gate the base build.

### Lintian

We aim for **zero errors** and minimal, justified warnings. The
acceptance bar for the personal PPA, mentors.debian.net, and the
NEW-queue review.

Common warnings we currently override:

- `package-contains-no-arch-dependent-files` for `intp-doc` (correct
  for a `Architecture: all` doc package).

Anything else gets an `intp.lintian-overrides` entry only if the
maintainer is confident it is wrong, with a comment explaining why.

### Personal PPA workflow

```bash
git checkout v0.1.0
gbp buildpackage --git-tag --git-pbuilder
dput ppa:<owner>/intp ../intp_0.1.0-1_source.changes
```

PPA configuration lives in `packaging/debian/ppa-config.yml` (TBD).

---

## Fedora / RHEL / CentOS Stream

### Source layout

`packaging/rpm/intp.spec` is a single-file spec.

```
%global meson_options -Dwith_tests=true -Dwith_examples=false

Name:           intp
Version:        0.1.0
Release:        1%{?dist}
Summary:        Linux interference profiler
License:        MIT
URL:            https://github.com/ggrv-intp/intp
Source0:        %{url}/archive/v%{version}/intp-%{version}.tar.gz

BuildRequires:  meson ninja-build clang
BuildRequires:  libbpf-devel bpftool
BuildRequires:  elfutils-libelf-devel zlib-devel
BuildRequires:  libmicrohttpd-devel
...

%package        libs
Summary:        Shared library for %{name}

%package        devel
Summary:        Development files for %{name}
```

Sub-packages mirror the Debian split (`intp-libs`, `intp-devel`,
`intp-doc`, plus per-plugin sub-packages).

### COPR → official Fedora

1. Push the spec + tarball to a COPR project owned by the maintainer.
2. Once COPR builds clean for current Fedora and EPEL, file a
   [Fedora package review](https://docs.fedoraproject.org/en-US/packaging-guidelines/).
3. Address review feedback; on approval, the package enters Fedora
   rawhide and stable releases follow.

EPEL builds (RHEL/AlmaLinux/Rocky) are derived from the Fedora
package via the standard `fedpkg` workflow.

---

## Arch Linux

### Source layout

`packaging/arch/PKGBUILD` is the standard PKGBUILD.

```bash
pkgname=intp
pkgver=0.1.0
pkgrel=1
pkgdesc="Linux interference profiler"
arch=('x86_64' 'aarch64')
url='https://github.com/ggrv-intp/intp'
license=('MIT')
depends=('libbpf' 'libelf' 'zlib' 'libmicrohttpd')
makedepends=('meson' 'ninja' 'clang' 'bpf')
source=("$url/archive/v$pkgver/$pkgname-$pkgver.tar.gz")
sha256sums=('SKIP')

build() {
    cd "$pkgname-$pkgver"
    meson setup build --buildtype=release --prefix=/usr
    meson compile -C build
}

check() {
    cd "$pkgname-$pkgver"
    meson test -C build
}

package() {
    cd "$pkgname-$pkgver"
    meson install -C build --destdir="$pkgdir"
    install -Dm644 LICENSE "$pkgdir/usr/share/licenses/$pkgname/LICENSE"
}
```

### AUR → official `extra`

1. Submit `intp` to AUR.
2. After it has been on AUR with active users for a release cycle or
   two, request a Trusted User to adopt it into the `extra`
   repository. Some TU activity is unavoidable here; the
   [Arch packaging guidelines](https://wiki.archlinux.org/title/Arch_package_guidelines)
   are the binding spec.

---

## Alpine

`packaging/alpine/APKBUILD` follows Alpine conventions. Two
gotchas:

- **musl-clean:** any glibc-only call site (e.g. `strlcpy` was
  glibc-only until recently) breaks here first. CI runs an Alpine
  build to catch this.
- **No SystemTap:** confirm at the `APKBUILD` level that we never
  pull in stap-related deps even transitively.

Submit via the standard `aports` workflow.

---

## Nix / NixOS

`packaging/nix/default.nix` is a standard `mkDerivation`. A `flake.nix`
lands post-1.0. The hermetic nature of Nix builds is a forcing
function for build-determinism cleanups; we welcome bug reports here.

---

## Static-binary release

GitHub Actions release workflow builds:

- `intp-static-x86_64-linux.tar.gz` — fully static, glibc-linked.
- `intp-static-aarch64-linux.tar.gz` — same, cross-built.
- `intp-static-x86_64-linux-musl.tar.gz` — musl-static.

These are signed with a release-time GPG key (the public key is in
the GitHub release notes) and are intended as *fallbacks*. Users
running these miss out on distro-managed updates; we recommend the
distro package whenever possible.

---

## When the upstream version doesn't match the distro version

Distros do not always update at the same cadence. We commit to:

- Backporting *security fixes* to the previous LTS-compatible major
  version for the lifetime of the supported Debian / Ubuntu LTS.
- Clearly tagging which `intp` version corresponds to which kernel
  ABI assumptions in [`docs/ARCHITECTURE.md`](ARCHITECTURE.md) and
  [`docs/METRICS.md`](METRICS.md).
- Not requiring a kernel newer than what the distro itself ships in
  any given LTS.

If a distro's shipped `intp` is incompatible with a feature you need,
the static binary or building from source remains an option. The
project will not ship "newer than the distro's kernel" requirements
in a way that breaks the distro package.

---

## Reach goals (post-1.0)

- **Snap** package — for users who want auto-updating but live on
  older LTS releases.
- **Flatpak** — less obviously a fit for a tool that needs host
  capabilities, but the integration model with Flatpak's permission
  system is worth exploring.
- **Container image** — a minimal `intp:latest` for use as a
  Kubernetes DaemonSet image, mirroring the Helm chart in
  `integrations/k8s/`.

These are not on the critical path; they will land as PRs from
maintainers who care about a specific channel.
