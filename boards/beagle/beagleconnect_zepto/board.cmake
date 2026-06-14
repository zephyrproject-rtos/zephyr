# BeagleBoard.org BeagleConnect Zepto
#
# SPDX-FileCopyrightText: Copyright 2026 BeagleBoard.org Foundation
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_MSPM0L1117)
  board_runner_args(openocd "--no-halt" "--no-targets" "--gdb-client-port=3339" "--config=${BOARD_DIR}/support/openocd.cfg")
  include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
endif()
