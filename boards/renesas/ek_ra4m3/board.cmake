# Copyright (c) 2024 Piotr Rak <piotr.rak@gmail.com>
# SPDX-License-Identifier: Apache-2.0

board_set_debugger_ifnset(jlink)
board_set_flasher_ifnset(jlink)

board_runner_args(jlink
  "--device=R7FA4M3AF"
  "--speed=4000"
  "--iface=SWD"
  "--reset-after-load")
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
