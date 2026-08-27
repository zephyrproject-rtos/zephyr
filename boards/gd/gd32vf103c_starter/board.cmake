# Copyright (c) 2022 YuLong Yao <feilongphone@gmail.com>
# SPDX-License-Identifier: Apache-2.0

board_runner_args(
  jlink
  "--device=GD32VF103CBT6" "--iface=jtag" "--tool-opt=-JTAGConf -1,-1"
)

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
