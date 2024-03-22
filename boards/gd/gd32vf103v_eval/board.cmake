# Copyright (c) 2021, Teslabs Engineering S.L.
# SPDX-License-Identifier: Apache-2.0

board_runner_args(
  jlink
  "--device=GD32VF103VBT6" "--iface=jtag" "--tool-opt=-JTAGConf -1,-1"
)

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
