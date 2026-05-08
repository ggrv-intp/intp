libintp_sources = files(
        'libintp.c',
)

libintp = library(
        'intp',
        libintp_sources,
        version: meson.project_version(),
        soversion: '0',
        include_directories: intp_include,
        c_args: ['-fvisibility=hidden'],
        gnu_symbol_visibility: 'hidden',
        install: true,
)

libintp_dep = declare_dependency(
        link_with: libintp,
        include_directories: intp_include,
)
