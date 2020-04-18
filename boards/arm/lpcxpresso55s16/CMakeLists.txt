#
# Copyright (c) 2020 Henrik Brix Andersen <henrik@brixandersen.dk>
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_PINMUX_MCUX_LPC)
  zephyr_library()
  zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)
  zephyr_library_sources(pinmux.c)
endif()
