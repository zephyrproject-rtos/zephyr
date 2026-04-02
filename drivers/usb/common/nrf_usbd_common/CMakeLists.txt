# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_NRF_USBD_COMMON)
  zephyr_library()

  zephyr_include_directories(.)

  zephyr_library_sources(nrf_usbd_common.c)
endif()
