# Copyright (c) 2024 Microchip

# SPDX-License-Identifier: Apache-2.0

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
board_finalize_runner_args(jlink "--device=pic32cx1025sg41128")
