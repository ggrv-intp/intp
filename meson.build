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

# libbpf is required only when eBPF is enabled. `auto` resolves here:
# present at configure time → enabled, absent → disabled.
libbpf_dep = dependency('libbpf', version: '>= 0.8',
                        required: opt_ebpf == 'enabled')
have_libbpf = libbpf_dep.found()

if opt_ebpf == 'auto'
        ebpf_enabled = have_libbpf
elif opt_ebpf == 'enabled'
        ebpf_enabled = true
else
        ebpf_enabled = false
endif

if ebpf_enabled and not have_libbpf
        error('--with-ebpf=enabled was requested but libbpf was not found')
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
