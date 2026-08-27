# board.cmake for am62x_m4_bl350
#
# Copyright (c) 2026 Chrispine Tinega <dev@chrispinetinega.com>
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_AM6254_M4)
  # Use a dedicated GDB client port for the M4F core; OpenOCD config
  # for AM62x maps this to the Cortex-M4F debug target.
  board_runner_args(openocd "--no-init" "--no-halt" "--no-targets" "--gdb-client-port=3339")
  include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
endif()
