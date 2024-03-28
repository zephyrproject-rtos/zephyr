# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

if (CONFIG_BOARD_NRF54L15PDK_NRF54L15_CPUAPP)
  board_runner_args(jlink "--device=cortex-m33" "--speed=4000")
elseif (CONFIG_BOARD_NRF54L15PDK_NRF54L15_CPUFLPR)
  board_runner_args(jlink "--speed=4000")
endif()

include(${ZEPHYR_BASE}/boards/common/nrfjprog.board.cmake)
include(${ZEPHYR_BASE}/boards/common/nrfutil.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
