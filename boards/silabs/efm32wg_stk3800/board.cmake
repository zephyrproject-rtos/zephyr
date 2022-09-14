# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=EFM32WG990F256")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
