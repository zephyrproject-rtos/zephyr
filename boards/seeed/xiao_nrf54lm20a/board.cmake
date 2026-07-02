# Copyright (c) 2025 Seeed Technology Co., Ltd.
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_NRF54LM20A_CPUAPP)
  board_runner_args(openocd "--cmd-load=nrf54lm20a-load" -c "targets nrf54lm20a.cpu")
  board_runner_args(jlink "--device=nRF54LM20A_M33" "--speed=4000")
elseif(CONFIG_SOC_NRF54LM20A_CPUFLPR)
  board_runner_args(openocd "--cmd-load=nrf54lm20a-load" -c "targets nrf54lm20a.aux")
  board_runner_args(jlink "--device=nRF54LM20A_RV32" "--speed=4000")
endif()

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/nrfutil.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
