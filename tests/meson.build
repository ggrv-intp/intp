# tests/meson.build — top-level test orchestration.
#
# Three suites, all run by `meson test -C build` by default:
#
#   - unit         hermetic, fast, no privileges, no real hardware.
#   - plugin-abi   loads examples/plugin-hello-world via dlopen and
#                  asserts the descriptor contract.
#   - integration  CLI-level smoke tests that exercise the actual
#                  intp binary against canned fixtures.
#
# A fourth suite (`live`) lives under tests/integration/live/ and
# requires root + a real kernel; it is opt-in via
# `meson test --suite live` and is not run by default.

# Common helper sources (fixture loader, tiny assertion macros).
test_common_inc = include_directories('common')

subdir('unit')
subdir('plugin-abi')
subdir('integration')
