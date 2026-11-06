# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_MIKROE_UNI_CLICKER_STM32F429XX_MCU_CARD_4)
  board_runner_args(jlink "--device=STM32F429NI" "--speed=4000")
endif()

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
