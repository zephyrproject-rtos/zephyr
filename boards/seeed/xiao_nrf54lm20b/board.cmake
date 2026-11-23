# Copyright (c) 2026 Seeed Technology Co., Ltd.
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_NRF54LM20B_CPUAPP)
  board_runner_args(jlink "--device=nRF54LM20A_M33" "--speed=4000")
endif()

include(${ZEPHYR_BASE}/boards/common/nrfutil.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
