#
# Copyright (c) 2020, Bernhard Kraemer
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_PINMUX)
    zephyr_library()
    zephyr_library_sources(pinmux.c)
    zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)
endif()

zephyr_library_sources(teensy4_flexspi_nor_config.c)
zephyr_library_sources_ifdef(CONFIG_DEVICE_CONFIGURATION_DATA teensy4_sdram_ini_dcd.c)
