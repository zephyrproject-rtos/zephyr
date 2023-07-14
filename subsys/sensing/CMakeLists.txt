# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(
    src/data_transform.c
    src/processing.c
    src/sensor_arbitrate.c
    src/sensor_connections.c
    src/sensor_info.c
    src/sensor_pipe.c
)
zephyr_library_sources_ifdef(CONFIG_SENSING_SHELL src/shell.c)
zephyr_library_sources_ifdef(CONFIG_USERSPACE     src/userspace.c)

zephyr_library_include_directories(include)
add_subdirectory(vsensors)

zephyr_linker_sources(DATA_SECTIONS sections.ld)
