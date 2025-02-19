# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_MISC_NORDIC)
  zephyr_library()
  zephyr_library_sources_ifdef(CONFIG_NORDIC_NRF_CPU nrf_cpu.c)
  zephyr_library_sources_ifdef(CONFIG_NORDIC_NRF_POWER nrf_power.c)
  zephyr_library_sources_ifdef(CONFIG_NORDIC_NRF_LRCCONF nrf_lrcconf.c)
endif()
