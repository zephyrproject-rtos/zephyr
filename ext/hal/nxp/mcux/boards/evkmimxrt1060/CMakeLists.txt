#
# Copyright (c) 2018, NXP
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_compile_definitions_ifdef(CONFIG_NXP_IMX_RT_BOOT_HEADER XIP_BOOT_HEADER_ENABLE=1)

zephyr_sources_ifdef(CONFIG_BOOT_FLEXSPI_NOR evkmimxrt1060_flexspi_nor_config.c)
