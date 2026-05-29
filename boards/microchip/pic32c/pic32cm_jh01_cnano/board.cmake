# Copyright (c) 2025 Microchip Technology Inc.
# SPDX-License-Identifier: Apache-2.0

board_runner_args(pyocd "--target=pic32cm5164jh01048" "--frequency=4000")
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
