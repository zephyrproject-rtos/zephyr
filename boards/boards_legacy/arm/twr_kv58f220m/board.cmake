# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=MKV58F1M0xxx24")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
