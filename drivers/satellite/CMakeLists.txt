# Copyright (c) 2022 Kickmaker
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SATELLITE_KIM1)
  zephyr_library_include_directories(${ZEPHYR_BASE}/drivers/modem)
  zephyr_library_sources(kim1.c)
endif()
