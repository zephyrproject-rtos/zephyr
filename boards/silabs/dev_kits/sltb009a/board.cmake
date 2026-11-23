# Copyright (c) 2023 Antmicro <www.antmicro.com>
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=EFM32GG12B810F1024")
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
