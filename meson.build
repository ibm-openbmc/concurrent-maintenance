project(
    'concurrent-maintenance',
    'cpp',
    version: '0.1',
    meson_version: '>=1.4.0',
    default_options: ['cpp_std=c++23', 'warning_level=3', 'werror=true'],
)

sdbusplus_dep = dependency('sdbusplus')
phosphor_logging_dep = dependency('phosphor-logging')
systemd_dep = dependency('systemd')

inc = include_directories('include')

executable(
    'ibm-concurrent-maintenance',
    ['src/main.cpp', 'src/manager.cpp'],
    include_directories: inc,
    dependencies: [sdbusplus_dep, phosphor_logging_dep],
    install: true,
)

systemd_system_unit_dir = systemd_dep.get_variable(
    pkgconfig: 'systemdsystemunitdir',
)

install_data(
    'service_files/com.ibm.ConcurrentMaintenance.service',
    install_dir: systemd_system_unit_dir,
)
