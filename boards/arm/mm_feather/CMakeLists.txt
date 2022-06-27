#
# Copyright (c) 2021 MADMACHINE LIMITED
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_sources_ifdef(CONFIG_BOOT_FLEXSPI_NOR flexspi_nor_config.c)
zephyr_sources_ifdef(CONFIG_DEVICE_CONFIGURATION_DATA mmfeather_sdram_ini_dcd.c)
