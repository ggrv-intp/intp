# tests/plugin-abi/meson.build — verifies the hello-world plugin
# satisfies the plugin-ABI contract.
#
# This suite is the *contract test* for include/intp/plugin.h. Any
# change that breaks the assertions here is, by definition, breaking
# the plugin ABI; the abidiff CI gate complements this from the
# binary side.

if get_option('with_examples')
        # The hello-world plugin .so is built in examples/plugin-hello-world.
        # We dlopen it from the test, so we need its build path.
        hello_world_so = 'examples/plugin-hello-world/intp-plugin-hello-world.so'

        test_pabi_descriptor = executable(
                'test-pabi-descriptor',
                'test_descriptor.c',
                include_directories: [intp_include, test_common_inc],
                link_with: libintp,
                dependencies: [cc.find_library('dl', required: false)],
                install: false,
        )

        test('plugin-abi/hello-world-descriptor',
             test_pabi_descriptor,
             suite: 'plugin-abi',
             args: [meson.project_build_root() / hello_world_so],
        )
else
        message('plugin-abi suite skipped (with_examples=false).')
endif
