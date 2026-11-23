# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=STM32WLE5CC" "--speed=4000" "--reset-after-load")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
