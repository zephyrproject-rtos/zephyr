# Copyright (c) 2019 Nordic Semiconductor ASA.
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_NRF9160DK_NRF9160 OR CONFIG_BOARD_NRF9160DK_NRF9160_NS)
  zephyr_library()
  zephyr_library_sources(nrf52840_reset.c)
elseif(CONFIG_BOARD_NRF9160DK_NRF52840)
  zephyr_library()
  zephyr_library_sources(board.c)
endif()
