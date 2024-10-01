# Copyright 2024 NXP
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)
zephyr_library()

zephyr_library_sources(clock_management_common.c)

# Include headers for all clock management drivers, registered with add_clock_management_header
zephyr_library_compile_options("-include$<JOIN:$<TARGET_PROPERTY:clock_management_header_target,INTERFACE_SOURCES>,;-include>")
