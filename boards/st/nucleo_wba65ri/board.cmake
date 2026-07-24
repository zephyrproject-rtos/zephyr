# Copyright (c) 2025 STMicroelectronics
# SPDX-License-Identifier: Apache-2.0

# keep first
board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw")
if(CONFIG_BUILD_WITH_TFM)
  board_runner_args(stm32cubeprogrammer "--erase")
endif()

include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd-stm32.board.cmake)
include(${ZEPHYR_BASE}/boards/st/common/CMakeLists.txt)
