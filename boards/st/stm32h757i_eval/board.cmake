# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2019 STMicroelectronics
# Copyright (c) 2025 Foss Analytical A/S

# keep first
board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw")

if(CONFIG_BOARD_STM32H757I_EVAL_STM32H757XX_M7)
  board_runner_args(jlink "--device=STM32H757XI")
  board_runner_args(openocd "--config=${BOARD_DIR}/support/openocd_stm32h757i_eval_m7.cfg")
  board_runner_args(openocd --target-handle=_CHIPNAME.cpu0)
elseif(CONFIG_BOARD_STM32H757I_EVAL_STM32H757XX_M4)
  board_runner_args(jlink "--device=STM32H757XI_M4")
  board_runner_args(openocd "--config=${BOARD_DIR}/support/openocd_stm32h757i_eval_m4.cfg")
  board_runner_args(openocd --target-handle=_CHIPNAME.cpu1)
endif()

# keep first
include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
