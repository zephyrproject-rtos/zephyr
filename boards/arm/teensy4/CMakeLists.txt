#
# Copyright (c) 2020, Bernhard Kraemer
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library()
zephyr_library_sources(flexspi_nor_config.c)
zephyr_library_sources_ifdef(CONFIG_DEVICE_CONFIGURATION_DATA teensy4_sdram_ini_dcd.c)
