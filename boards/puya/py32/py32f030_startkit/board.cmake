#
# Copyright (c) 2019 Antony Pavlov <antonynpavlov@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0
#

board_runner_args(jlink
  "--device=PY32F030X6"
  "--iface=swd"
  "--speed=1000"
  "--reset-type=0"
  "--reset"
)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
