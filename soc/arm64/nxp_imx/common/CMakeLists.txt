# Copyright 2023 NXP
#
# SPDX-License-Identifier: Apache-2.0

zephyr_interface_library_named(NXP_IMX_COMMON)

zephyr_library_named(nxp_imx_common)
zephyr_library_include_directories(include)
zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)

zephyr_library_link_libraries(NXP_IMX_COMMON)

target_include_directories(NXP_IMX_COMMON INTERFACE include)
target_link_libraries(NXP_IMX_COMMON INTERFACE nxp_imx_common)
