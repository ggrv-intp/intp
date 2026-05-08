project(
        'intp',
        'c',
        version: '0.1.0',
        license: 'MIT',
        meson_version: '>= 0.61.0',
        default_options: [
                'c_std=c11',
                'warning_level=2',
                'buildtype=debugoptimized',
                'b_ndebug=if-release',
                'b_lundef=true',
                'b_pie=true',
        ],
)

# ---------------------------------------------------------------------------
# Compiler setup
# ---------------------------------------------------------------------------

cc = meson.get_compiler('c')

add_project_arguments(
        cc.get_supported_arguments([
                '-Wmissing-prototypes',
                '-Wstrict-prototypes',
                '-Wshadow',
                '-Wpointer-arith',
                '-Wcast-qual',
                '-Wcast-align',
                '-Wwrite-strings',
                '-Wundef',
                '-Wbad-function-cast',
                '-Wmissing-declarations',
                '-Wno-unused-parameter',
                '-D_GNU_SOURCE',
                '-D_FILE_OFFSET_BITS=64',
        ]),
        language: 'c',
)

# ---------------------------------------------------------------------------
# Build options
# ---------------------------------------------------------------------------

opt_ebpf       = get_option('with_ebpf')
opt_bpftrace   = get_option('with_bpftrace')
opt_gpu_nvml   = get_option('with_gpu_nvml')
opt_gpu_rocm   = get_option('with_gpu_rocm')
opt_gpu_zes    = get_option('with_gpu_zes')
opt_otel       = get_option('with_otel')
opt_prometheus = get_option('with_prometheus')
opt_tests      = get_option('with_tests')
opt_examples   = get_option('with_examples')
opt_static     = get_option('with_static')

# ---------------------------------------------------------------------------
# Optional dependencies
# ---------------------------------------------------------------------------

# libbpf is required only when eBPF is enabled. We probe without a
# version constraint and check it manually so we can produce a clear
# error / message when the version is too old (Ubuntu 22.04 ships
# libbpf 0.5; we need >= 0.8 per ADR-0003). See docs/BUILDING.md for
# the PKG_CONFIG_PATH override path.
libbpf_dep = dependency('libbpf', required: false)
libbpf_min = '>= 0.8'
libbpf_ok = libbpf_dep.found() and libbpf_dep.version().version_compare(libbpf_min)

if opt_ebpf == 'enabled' and not libbpf_ok
        if libbpf_dep.found()
                error('with_ebpf=enabled but libbpf @0@ found, need @1@'.format(
                        libbpf_dep.version(), libbpf_min))
        else
                error('with_ebpf=enabled but libbpf not found')
        endif
endif

if opt_ebpf == 'auto'
        ebpf_enabled = libbpf_ok
elif opt_ebpf == 'enabled'
        ebpf_enabled = true
else
        ebpf_enabled = false
endif

# When auto resolves to disabled, explain why instead of being silent —
# users on older LTS releases otherwise wonder why eBPF isn't on.
if opt_ebpf == 'auto' and not ebpf_enabled
        if libbpf_dep.found()
                message('intp: found libbpf @0@ but @1@ required.'.format(
                        libbpf_dep.version(), libbpf_min))
                message('intp: eBPF backends disabled; pure-C path will be used at runtime.')
                message('intp: -- to enable, install a newer libbpf or set PKG_CONFIG_PATH (see docs/BUILDING.md).')
                message('intp: -- to fail loud instead, configure with -Dwith_ebpf=enabled.')
        else
                message('intp: libbpf not found; eBPF backends disabled (pure-C path).')
                message('intp: -- to enable, install libbpf-dev (@0@) (see docs/BUILDING.md).'.format(libbpf_min))
        endif
endif

# Prometheus exposition needs an HTTP server. libmicrohttpd is the default.
mhd_dep = dependency('libmicrohttpd', required: opt_prometheus)
prometheus_enabled = mhd_dep.found() and opt_prometheus

# OpenTelemetry needs protobuf + (optionally) grpc.
protobuf_dep = dependency('protobuf', required: opt_otel)
otel_enabled = protobuf_dep.found() and opt_otel

# ---------------------------------------------------------------------------
# Configuration data (consumed by include/intp/meson.build to generate
# the version header and exported through it).
# ---------------------------------------------------------------------------

conf_data = configuration_data()
conf_data.set_quoted('INTP_VERSION', meson.project_version())
conf_data.set_quoted('INTP_PREFIX', get_option('prefix'))
conf_data.set('INTP_HAVE_EBPF',         ebpf_enabled        ? '1' : '0')
conf_data.set('INTP_HAVE_BPFTRACE',     opt_bpftrace        ? '1' : '0')
conf_data.set('INTP_HAVE_GPU_NVML',     opt_gpu_nvml        ? '1' : '0')
conf_data.set('INTP_HAVE_GPU_ROCM',     opt_gpu_rocm        ? '1' : '0')
conf_data.set('INTP_HAVE_GPU_ZES',      opt_gpu_zes         ? '1' : '0')
conf_data.set('INTP_HAVE_OTEL',         otel_enabled        ? '1' : '0')
conf_data.set('INTP_HAVE_PROMETHEUS',   prometheus_enabled  ? '1' : '0')

# ---------------------------------------------------------------------------
# Public headers — including version.h generated from version.h.in.
# Lives in a subdir so the generated file lands at
# build/include/intp/version.h, where <intp/version.h> finds it.
# ---------------------------------------------------------------------------

subdir('include/intp')

# Make include/ visible everywhere; meson automatically also exposes
# the build-tree mirror (where the generated version.h lives).
intp_include = include_directories('include')

# ---------------------------------------------------------------------------
# Subdirs (libintp, intp CLI, examples, tests)
# ---------------------------------------------------------------------------

subdir('src/lib')
subdir('src/cli')
subdir('src/plugins/bpftrace')

if opt_examples
        subdir('examples')
endif

if opt_tests
        subdir('tests')
endif

# ---------------------------------------------------------------------------
# pkg-config
# ---------------------------------------------------------------------------

pkg = import('pkgconfig')
pkg.generate(
        libintp,
        name: 'intp',
        description: 'Linux interference profiler — public C API',
        version: meson.project_version(),
        url: 'https://github.com/ggrv-intp/intp',
        subdirs: ['intp'],
)

# ---------------------------------------------------------------------------
# Configuration summary
# ---------------------------------------------------------------------------

summary({
        'eBPF backends'       : ebpf_enabled,
        'bpftrace plugin'     : opt_bpftrace,
        'GPU NVML plugin'     : opt_gpu_nvml,
        'GPU ROCm plugin'     : opt_gpu_rocm,
        'GPU Level Zero plug.': opt_gpu_zes,
        'OpenTelemetry sink'  : otel_enabled,
        'Prometheus sink'     : prometheus_enabled,
        'Tests'               : opt_tests,
        'Examples'            : opt_examples,
        'Static binary'       : opt_static,
}, section: 'Build options')
