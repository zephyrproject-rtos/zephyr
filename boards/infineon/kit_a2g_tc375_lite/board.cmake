# Copyright (c) 2026 Infineon Technologies AG
#
# SPDX-License-Identifier: Apache-2.0

board_runner_args(openocd
  "--config=board/infineon/kit_a2g_tc375_lite.cfg"
  "--cmd-pre-init=gdb_port disabled"
  "--cmd-pre-init=tcl_port disabled"
  "--cmd-pre-init=telnet_port disabled"
  "--cmd-reset-halt=reset halt"
  "--cmd-load=program"
  "--no-halt"
  "--no-targets"
)

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
