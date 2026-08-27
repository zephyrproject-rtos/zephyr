# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_THINGY53_NRF5340_CPUAPP OR CONFIG_BOARD_THINGY53_NRF5340_CPUAPP_NS)
  zephyr_library()
  zephyr_library_sources(board.c)
endif()
