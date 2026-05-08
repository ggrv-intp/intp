# tests/integration/meson.build — CLI-level smoke tests against the
# `intp` binary as built. Each test invokes the binary with specific
# flags and asserts on its exit code (and, optionally, its output).

# `intp --version` exits 0 and prints something sensible.
test('cli/version-exits-0',
     find_program('sh'),
     args: ['-c', '@0@ --version | grep -q "^intp "'.format(meson.project_build_root() / 'src/cli/intp')],
     suite: 'integration',
)

# `intp --help` exits 0 and describes the tool.
test('cli/help-exits-0',
     find_program('sh'),
     args: ['-c',
            '@0@ --help | grep -q "interference profiler"'.format(
                meson.project_build_root() / 'src/cli/intp')],
     suite: 'integration',
)

# `intp --print-schema` exits 0 and lists the seven core metrics.
test('cli/print-schema-lists-7-core-metrics',
     find_program('sh'),
     args: ['-c',
            '@0@ --print-schema | grep -c "system.interference\\." | grep -qx 7'.format(
                meson.project_build_root() / 'src/cli/intp')],
     suite: 'integration',
)

# Live tests (require root + real kernel) live under live/ and are
# opted into with `meson test --suite live`. They are intentionally
# *not* added to the default suite. Until live/ has cases this is
# a placeholder.
# subdir('live')
