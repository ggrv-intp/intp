libintp_sources = files(
        'libintp.c',
        'probe.c',
        'prometheus.c',
)

# Conditional link deps: libbpf for the eBPF runtime probe, libmicrohttpd
# for the Prometheus exposition server. Both populate libintp1's runtime
# Depends automatically via dh_shlibdeps when the corresponding option
# is enabled.
libintp_deps = []
if ebpf_enabled
        libintp_deps += libbpf_dep
endif
if prometheus_enabled
        libintp_deps += mhd_dep
endif

libintp = library(
        'intp',
        libintp_sources,
        version: meson.project_version(),
        soversion: '0',
        include_directories: intp_include,
        c_args: ['-fvisibility=hidden'],
        gnu_symbol_visibility: 'hidden',
        dependencies: libintp_deps,
        install: true,
)

libintp_dep = declare_dependency(
        link_with: libintp,
        include_directories: intp_include,
)
