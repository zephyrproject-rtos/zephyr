# SPDX-License-Identifier: Apache-2.0

# keep first
board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw")
board_runner_args(jlink "--device=STM32H755ZI" "--speed=3300")
# Connect under reset when flashing firmware that disables debug during sleep.
board_runner_args(openocd --cmd-pre-init-flash "reset_config connect_assert_srst")
if(CONFIG_BOARD_NUCLEO_H755ZI_Q_STM32H755XX_M7)
  board_runner_args(openocd --target-handle=_CHIPNAME.cpu0)
elseif(CONFIG_BOARD_NUCLEO_H755ZI_Q_STM32H755XX_M4)
  board_runner_args(openocd --target-handle=_CHIPNAME.cpu1)
endif()

# keep first
include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd-stm32.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
