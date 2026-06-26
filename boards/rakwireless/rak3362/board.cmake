# Copyright (c) 2026 RAKwireless Technology Co., Ltd. <www.rakwireless.com>
# Leo Li <leo.li@rakwireless.com>
# SPDX-License-Identifier: Apache-2.0
board_runner_args(pyocd "--target=nrf54l")

if(CONFIG_SOC_NRF54L15_CPUAPP)
  board_runner_args(jlink "--device=nRF54L15_M33" "--speed=4000")
endif()

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/nrfutil.board.cmake)
include(${ZEPHYR_BASE}/boards/common/nrfjprog.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)