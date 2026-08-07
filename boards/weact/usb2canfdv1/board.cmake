# Copyright (c) 2024 Henrik Brix Andersen <henrik@brixandersen.dk>
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=STM32G0B1CB")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
