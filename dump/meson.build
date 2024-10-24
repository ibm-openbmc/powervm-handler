sdbusplus_dep = dependency(
    'sdbusplus',
    fallback: ['sdbusplus', 'sdbusplus_dep'],
)

sdeventplus_dep = dependency(
    'sdeventplus',
    fallback: ['sdeventplus', 'sdeventplus_dep'],
)

phosphor_dbus_interfaces_dep = dependency(
    'phosphor-dbus-interfaces',
    fallback: [
        'phosphor-dbus-interfaces',
        'phosphor_dbus_interfaces_dep'
    ],
)

phosphor_logging_dep = dependency(
    'phosphor-logging',
    fallback: ['phosphor-logging', 'phosphor_logging_dep'],
)

systemd_dep = dependency('systemd')

libpldm_dep = dependency(
    'libpldm',
    default_options: ['oem-ibm=enabled'],
)

configure_file(
    input: 'config.h.in',
    output: 'config.h',
    configuration: configuration_data(),
)

dump_offload_deps = [
    phosphor_dbus_interfaces_dep,
    phosphor_logging_dep,
    sdbusplus_dep,
    sdeventplus_dep,
    libpldm_dep,
]

subdir('dist')

executable(
    'pvm_dump_offload',
    'host_offload_main.cpp',
    'offload_manager.cpp',
    'offload_handler.cpp',
    'dbus_util.cpp',
    'pldm_utils.cpp',
    'dump_watch.cpp',
    'dbus_util.cpp',
    'send_pldm_cmd.cpp',
    'pldm_oem_cmds.cpp',
    'host_offloader_queue.cpp',
    'host_state_watch.cpp',
    'hmc_state_watch.cpp',
    dependencies: dump_offload_deps,
    install: true,
)
