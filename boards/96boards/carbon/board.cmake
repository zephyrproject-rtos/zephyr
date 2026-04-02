# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_96B_CARBON_STM32F401XE)
  board_runner_args(dfu-util "--pid=0483:df11" "--alt=0" "--dfuse")

  include(${ZEPHYR_BASE}/boards/common/dfu-util.board.cmake)
endif()
