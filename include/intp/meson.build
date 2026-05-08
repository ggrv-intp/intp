public_headers = files(
        'intp.h',
        'metrics.h',
        'plugin.h',
        'schema.h',
)

version_h = configure_file(
        input:         'version.h.in',
        output:        'version.h',
        configuration: conf_data,
        install_dir:   get_option('includedir') / 'intp',
        install:       true,
)

install_headers(public_headers, subdir: 'intp')
