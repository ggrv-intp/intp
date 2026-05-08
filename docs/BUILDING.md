# Building `intp`

This document covers building from source. For installing pre-built
binaries, see your distribution's package manager —
[ROADMAP.md](ROADMAP.md) tracks the rollout.

---

## At a glance

```bash
git clone https://github.com/ggrv-intp/intp.git
cd intp
meson setup build
meson compile -C build
sudo meson install -C build      # optional
```

That is the whole build, on a host that has the build dependencies.
The rest of this document is about *how to get those dependencies*.

---

## Build dependencies

### Always required

- `meson` ≥ 0.61 — main build system.
- `ninja` ≥ 1.10 — backend.
- A C compiler — `gcc` ≥ 9 or `clang` ≥ 12.
- `pkg-config` — finds dependent libraries.

### Required when `--with-ebpf` is enabled (default: auto)

- `clang` ≥ 12 — to compile eBPF programs (must be `clang`, not
  `gcc`; the eBPF backend is not supported by gcc).
- `bpftool` — to generate libbpf skeletons and dump `vmlinux.h`.
  Usually shipped with `linux-tools-<kernel>` (Debian/Ubuntu) or
  `bpftool` (Fedora/Arch).
- `libbpf` ≥ 0.8 with development headers (`libbpf-dev` or
  equivalent).
- `libelf` (build dependency of `libbpf`).
- `zlib` (build dependency of `libbpf`).
- A kernel ≥ 5.8 with BTF on the build host **or** a vmlinux BTF
  blob accessible at build time. (Runtime kernel can be different;
  CO-RE handles relocation.)

### Required when `--with-prometheus` is enabled

- `libmicrohttpd` (or your chosen HTTP server library) — for the
  `/metrics` endpoint when running in serve mode.

### Required when `--with-otel` is enabled

- `protobuf-compiler` and `libprotobuf-dev` — for OTLP message
  encoding.
- `grpc` and `libgrpc++-dev` — only when using OTLP/gRPC; OTLP/HTTP
  works without grpc.

### Required when `--with-tests` is enabled

- `criterion` (preferred) **or** the in-tree minimal harness —
  unit-test framework.
- `python3` ≥ 3.10 — fixture loader and orchestration scripts.
- `cppcheck`, `shellcheck`, `clang-format` ≥ 14 — for the lint pass.

---

## Distribution-specific install

### Debian / Ubuntu

```bash
sudo apt install -y \
    meson ninja-build pkg-config \
    gcc clang \
    libbpf-dev linux-tools-generic \
    libelf-dev zlib1g-dev \
    libmicrohttpd-dev \
    protobuf-compiler libprotobuf-dev \
    libgrpc++-dev \
    python3 python3-pytest \
    cppcheck shellcheck clang-format
```

For Debian 11 (bullseye) and older Ubuntu LTS releases, `libbpf-dev`
may be too old. Either upgrade libbpf via backports or build with
`--without-ebpf`.

### Fedora / RHEL / CentOS Stream

```bash
sudo dnf install -y \
    meson ninja-build pkgconf-pkg-config \
    gcc clang \
    libbpf-devel bpftool \
    elfutils-libelf-devel zlib-devel \
    libmicrohttpd-devel \
    protobuf-compiler protobuf-devel \
    grpc-devel grpc-cpp \
    python3 python3-pytest \
    cppcheck ShellCheck clang-tools-extra
```

### Arch Linux

```bash
sudo pacman -S --needed \
    meson ninja pkgconf \
    gcc clang \
    libbpf bpf \
    libelf zlib \
    libmicrohttpd \
    protobuf grpc \
    python python-pytest \
    cppcheck shellcheck clang
```

### Alpine

```bash
sudo apk add \
    meson ninja pkgconf \
    gcc clang \
    libbpf-dev bpftool \
    elfutils-dev zlib-dev \
    libmicrohttpd-dev \
    protobuf-dev grpc-dev \
    python3 py3-pytest \
    cppcheck shellcheck clang-extra-tools
```

Alpine uses musl. Some glibc-only paths in the procfs/sysfs parsers
will surface here first — please open a [packaging issue](../.github/ISSUE_TEMPLATE/distro-packaging.yml)
if you hit one.

### NixOS / Nix

```bash
nix-shell -p meson ninja pkg-config \
             gcc clang \
             libbpf bpftools \
             elfutils zlib \
             libmicrohttpd \
             protobuf grpc \
             python3 python3Packages.pytest \
             cppcheck shellcheck clang-tools
```

A `flake.nix` lands post-1.0; for now the shell above is the
recommended entry point.

---

## Build options (Meson)

```bash
meson setup build [options]
meson configure build               # show current values
meson configure build -Dwith_ebpf=disabled
```

| Option            | Default      | What it does                                       |
|-------------------|--------------|----------------------------------------------------|
| `with_ebpf`       | `auto`       | `enabled` / `disabled` / `auto` — eBPF backends.   |
| `with_bpftrace`   | `false`      | Build the bpftrace research plugin.                |
| `with_gpu_nvml`   | `false`      | NVIDIA GPU plugin (requires NVML SDK).             |
| `with_gpu_rocm`   | `false`      | AMD GPU plugin (requires ROCm).                    |
| `with_gpu_zes`    | `false`      | Intel GPU plugin (requires Level Zero).            |
| `with_otel`       | `false`      | OpenTelemetry OTLP output sink.                    |
| `with_prometheus` | `true`       | Prometheus exposition output sink.                 |
| `with_tests`      | `true`       | Build the test suite.                              |
| `with_examples`   | `false`      | Build `examples/` programs.                        |
| `with_static`     | `false`      | Static link `libintp` into the `intp` binary.      |
| `prefix`          | `/usr/local` | Install prefix.                                    |

A typical CI invocation: `meson setup build -Dwith_ebpf=enabled
-Dwith_tests=true -Dwerror=true`.

---

## Cross-compiling

Meson handles cross-compilation via *cross files*. We ship reference
cross files in `packaging/cross/`:

```bash
meson setup build-arm64 \
    --cross-file packaging/cross/aarch64-linux-gnu.ini
meson compile -C build-arm64
```

eBPF cross-builds need a cross-built `vmlinux.h` for the target
kernel. The CI matrix exercises `arm64` and `riscv64` against
generic Debian kernels; cross-building for a specific kernel is a
manual step.

---

## Running the test suite

```bash
meson test -C build                  # default suite
meson test -C build --suite live     # tests that need real hardware
meson test -C build -v <test-name>   # one specific test
```

The default suite is hermetic and runs in CI. The `live` suite needs
root, a recent kernel, and real `perf_event_open` access; it is
suitable for local sanity checks before a release.

---

## Installing

```bash
sudo meson install -C build
```

Default layout under `--prefix=/usr/local`:

```text
/usr/local/bin/intp
/usr/local/lib/libintp.so.0
/usr/local/lib/libintp.so.0.<minor>
/usr/local/include/intp/{intp,metrics,plugin,schema,version}.h
/usr/local/lib/pkgconfig/intp.pc
/usr/local/lib/intp/plugins/                       (empty by default)
/usr/local/share/man/man1/intp.1
/usr/local/share/man/man8/intp-collector.8         (when with_prometheus)
/usr/local/share/doc/intp/                         (this docs/ tree)
```

For distro packagers, the Debian rules file uses `--prefix=/usr` with
`--libdir=/usr/lib/x86_64-linux-gnu` (multi-arch). See
[PACKAGING.md](PACKAGING.md).

---

## Common build problems

**"libbpf X.Y found, need ≥ 0.8"** — your distro's `libbpf-dev` is too
old. This is the common case on Ubuntu 22.04 (libbpf 0.5), Debian 11,
and RHEL 8. With the default `-Dwith_ebpf=auto`, meson disables eBPF
quietly and prints an `intp: --` message at configure time; the
build still succeeds via the pure-C path. With `-Dwith_ebpf=enabled`,
meson fails loudly. Three ways forward, in order of recommendation:

1. **Build with `-Dwith_ebpf=disabled`** (production on older LTS).
   The pure-C backend covers all seven metrics with slightly lower
   fidelity on `blk` and `nets` — see
   [ADR-0003](design/0003-ebpf-acceleration.md) for the trade-off.
   This is what `intp` running in `apt install`-shipped form will
   look like on jammy.

2. **Use a newer libbpf via `PKG_CONFIG_PATH`** (development on
   older LTS, or research that needs the eBPF backend on jammy):

   ```bash
   # Build libbpf into a per-user prefix (no sudo required).
   git clone --branch v1.5.0 https://github.com/libbpf/libbpf.git ~/src/libbpf
   cd ~/src/libbpf/src
   DESTDIR=$HOME/.local/libbpf-1.5 PREFIX= LIBDIR=/lib make install

   # Point intp's meson at it.
   cd /path/to/intp
   PKG_CONFIG_PATH=$HOME/.local/libbpf-1.5/lib/pkgconfig \
   LD_LIBRARY_PATH=$HOME/.local/libbpf-1.5/lib \
       meson setup build -Dwith_ebpf=enabled
   meson compile -C build
   ```

   At runtime, the linker needs to find your custom `libbpf.so.1` —
   either via `LD_LIBRARY_PATH` as above, or by passing
   `-Dc_link_args=-Wl,-rpath,$HOME/.local/libbpf-1.5/lib` to
   `meson setup` so the rpath bakes into the binary.

3. **Install a distro backport** if one exists. Debian
   `bullseye-backports` and Ubuntu PPAs occasionally ship newer
   `libbpf-dev`; check `apt-cache policy libbpf-dev` and the
   relevant backports lists.

We deliberately do *not* vendor libbpf as a meson subproject:
Debian Policy §4.13 discourages embedded copies of system libraries,
and bundling would force every distro packager to disable the
fallback. The `PKG_CONFIG_PATH` override above gives the same
end-user benefit without that downside.

**"BTF not available on this host"** — the build host's running
kernel does not expose `/sys/kernel/btf/vmlinux`. The build can still
succeed if you point `bpftool btf dump file` at a vmlinux ELF you
have access to (`-Dvmlinux_btf=path/to/vmlinux`), or just disable
eBPF with `--without-ebpf`.

**"clang-format complains about everything"** — make sure you have
clang-format ≥ 14. Older versions ignore some of the kernel-style
options in `.clang-format`.

**"CMake complains, not Meson"** — wrong build system. We use Meson.
There is no `CMakeLists.txt`.

**"systemd unit file not installed"** — only when
`-Dwith_prometheus=true`. Check `meson configure build`.
