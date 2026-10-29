gtest_dep = dependency('gtest', main: true, required: get_option('tests'))

test_sources = [
    '../src/manager.cpp',
    '../src/cm_object.cpp',
    '../src/fru_identifier.cpp',
    '../src/cm_handlers.cpp',
]

test_manager = executable(
    'test-manager',
    ['test_manager.cpp'] + test_sources,
    include_directories: inc,
    dependencies: [
        gtest_dep,
        sdbusplus_dep,
        phosphor_logging_dep,
        phosphor_dbus_interfaces_dep,
    ],
)
test('manager', test_manager)

test_cm_object = executable(
    'test-cm-object',
    ['test_cm_object.cpp'] + test_sources,
    include_directories: inc,
    dependencies: [
        gtest_dep,
        sdbusplus_dep,
        phosphor_logging_dep,
        phosphor_dbus_interfaces_dep,
    ],
)
test('cm_object', test_cm_object)
