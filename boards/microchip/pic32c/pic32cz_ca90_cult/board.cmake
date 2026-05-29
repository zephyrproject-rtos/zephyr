# Copyright (c) 2026 Microchip Technology Inc.
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=PIC32CZ8110CA90" "--speed=4000")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
