# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=atsam4s16c" "--speed=4000")
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
