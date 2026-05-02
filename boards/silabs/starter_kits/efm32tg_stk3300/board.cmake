# Copyright (c) 2025, Lukas Woodtli
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=EFM32TG840F32")
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
