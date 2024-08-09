# Copyright 2024 NXP
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)
zephyr_library()

zephyr_library_sources(clock_mgmt_common.c)
zephyr_library_sources_ifdef(CONFIG_CLOCK_MGMT_FIXED_SOURCE fixed_clock_source.c)
zephyr_library_sources_ifdef(CONFIG_CLOCK_MGMT_OUTPUT clock_output.c)

add_subdirectory(nxp_syscon)
