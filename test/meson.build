gtest_dep = dependency('gtest', main: true, required: get_option('tests'))

test_manager = executable(
    'test-manager',
    ['test_manager.cpp', '../src/manager.cpp', '../src/cm_object.cpp'],
    include_directories: inc,
    dependencies: [gtest_dep, sdbusplus_dep, phosphor_logging_dep],
)

test('manager', test_manager)
