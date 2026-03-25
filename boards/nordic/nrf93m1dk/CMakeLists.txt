# Copyright (c) 2026 Nordic Semiconductor ASA.
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_NRF93M1DK_NRF54L15_CPUAPP
  OR (CONFIG_BOARD_NRF93M1DK_NRF54L15_CPUAPP_NS AND CONFIG_BUILD_WITH_TFM))
  zephyr_library()
  zephyr_library_sources(board.c)
endif()
