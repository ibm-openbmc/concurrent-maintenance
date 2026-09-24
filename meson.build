project(
    'concurrent-maintenance',
    'cpp',
    version: '0.1',
    meson_version: '>=1.4.0',
    default_options: ['cpp_std=c++23', 'warning_level=3', 'werror=true'],
)

sdbusplus_dep = dependency('sdbusplus')
phosphor_logging_dep = dependency('phosphor-logging')
phosphor_dbus_interfaces_dep = dependency('phosphor-dbus-interfaces')
systemd_dep = dependency('systemd')

inc = include_directories('include')

executable(
    'ibm-concurrent-maintenance',
    [
        'src/main.cpp',
        'src/manager.cpp',
        'src/cm_object.cpp',
        'src/fru_identifier.cpp',
    ],
    include_directories: inc,
    dependencies: [
        sdbusplus_dep,
        phosphor_logging_dep,
        phosphor_dbus_interfaces_dep,
    ],
    install: true,
)

systemd_system_unit_dir = systemd_dep.get_variable(
    pkgconfig: 'systemdsystemunitdir',
)

install_data(
    'service_files/com.ibm.ConcurrentMaintenance.service',
    install_dir: systemd_system_unit_dir,
)

if get_option('tests').enabled()
    subdir('test')
endif

# clang-tidy
run_target(
    'clang-tidy',
    command: [
        'run-clang-tidy',
        '-p',
        meson.project_build_root(),
        meson.project_source_root() + '/src',
        meson.project_source_root() + '/include',
    ],
)
