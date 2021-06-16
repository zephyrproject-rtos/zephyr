# Copyright (c) 2021, Sateesh Kotapati
#
# SPDX-License-Identifier: Apache-2.0
#

board_runner_args(jlink "--device=EFR32BG22C224F512IM40" "--reset-after-load")
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
