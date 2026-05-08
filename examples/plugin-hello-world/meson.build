shared_module(
        'intp-plugin-hello-world',
        'hello-world.c',
        include_directories: intp_include,
        # Plugins are loaded via dlopen but use libintp helpers
        # (intp_sample_append_*, intp_plugin_ctx_*). Linking against
        # libintp gives the .so a DT_NEEDED entry; the dynamic linker
        # reuses the host process's already-loaded libintp instead of
        # creating a second copy.
        link_with: libintp,
        c_args: ['-fvisibility=hidden'],
        gnu_symbol_visibility: 'hidden',
        name_prefix: '',           # produce intp-plugin-hello-world.so
                                   # rather than libintp-plugin-...
        install: false,            # examples are not installed by default
)
