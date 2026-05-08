test_unit_metrics = executable(
        'test-unit-metrics',
        'test_metrics.c',
        include_directories: [intp_include, test_common_inc],
        link_with: libintp,
        install: false,
)

test('metrics/core-metric-names',
     test_unit_metrics,
     suite: 'unit',
     env: { 'INTP_TEST_FIXTURE_DIR': meson.project_source_root() / 'tests/fixtures' },
)

test_unit_units = executable(
        'test-unit-units',
        'test_units.c',
        include_directories: [intp_include, test_common_inc],
        link_with: libintp,
        install: false,
)

test('metrics/unit-names',
     test_unit_units,
     suite: 'unit',
)

test_unit_version = executable(
        'test-unit-version',
        'test_version.c',
        include_directories: [intp_include, test_common_inc],
        link_with: libintp,
        install: false,
)

test('lib/version-string',
     test_unit_version,
     suite: 'unit',
)
