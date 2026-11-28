# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink
  "--device=PY32F003X6"
  "--iface=swd"
  "--speed=1000"
  "--reset-type=0"
  "--reset"
)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
