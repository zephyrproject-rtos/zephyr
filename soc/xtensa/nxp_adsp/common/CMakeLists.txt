# NXP SoC family CMake file
#
# Copyright (c) 2021 NXP
# SPDX-License-Identifier: Apache-2.0

zephyr_interface_library_named(NXP_ADSP_COMMON)

zephyr_library_named(nxp_adsp_common)
zephyr_library_include_directories(include)
zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)

zephyr_library_sources(soc.c)

zephyr_library_link_libraries(NXP_ADSP_COMMON)

target_include_directories(NXP_ADSP_COMMON INTERFACE include)
target_link_libraries(NXP_ADSP_COMMON INTERFACE nxp_adsp_common)
