#
# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=MIMX9131" "--no-reset" "--flash-sram")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
