# Copyright (c) 2019 Nordic Semiconductor ASA.
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_NRF52840_GPIO_RESET)
  zephyr_library()
  zephyr_library_sources(nrf52840_reset.c)
endif()
