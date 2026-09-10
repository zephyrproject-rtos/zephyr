# BeagleBoard.org PocketBeagle 2
#
# Copyright (c) 2025 Ayush Singh, BeagleBoard.org Foundation
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_AM6232_M4)
  board_runner_args(openocd "--no-init" "--no-halt" "--no-targets" "--gdb-client-port=3339"
  "--config=${BOARD_DIR}/support/m4.cfg")
  include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
endif()

if(CONFIG_SOC_MSPM0L1105)
  board_runner_args(openocd "--no-init" "--no-halt" "--no-targets" "--gdb-client-port=3339"
  "--config=${BOARD_DIR}/support/mspm0l1105.cfg")
  include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
endif()

if(CONFIG_SOC_AM6254_M4)
  board_runner_args(openocd "--no-init" "--no-halt" "--no-targets" "--gdb-client-port=3339")
  include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
endif()
