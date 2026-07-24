# SPDX-License-Identifier: Apache-2.0

# keep first
board_runner_args(jlink "--device=STM32H573II" "--reset-after-load")

board_runner_args(pyocd "--target=stm32h573iikx")

board_runner_args(probe_rs "--chip=STM32H573II")

# keep first
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/probe-rs.board.cmake)
