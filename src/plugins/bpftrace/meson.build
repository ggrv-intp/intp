if opt_bpftrace
        shared_module(
                'bpftrace',
                'bpftrace_plugin.c',
                include_directories: intp_include,
                # See examples/plugin-hello-world/meson.build for why
                # plugins link_with libintp.
                link_with: libintp,
                c_args: ['-fvisibility=hidden'],
                gnu_symbol_visibility: 'hidden',
                name_prefix: '',
                install: true,
                install_dir: get_option('libdir') / 'intp' / 'plugins',
        )
endif
