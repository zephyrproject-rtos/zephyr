# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=MK66FN2M0xxx18")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
