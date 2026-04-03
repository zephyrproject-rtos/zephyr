# Copyright 2023 NXP
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink
  "--device=S32K148"
  "--speed=4000"
  "--iface=jtag"
  "--reset"
  "--tool-opt=-jtagconf -1,-1"
)

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
