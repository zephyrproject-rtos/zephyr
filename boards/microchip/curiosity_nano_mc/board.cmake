# Copyright (c) 2025 GP Orcullo
# SPDX-License-Identifier: Apache-2.0

board_runner_args(pyocd "--target=pic32cm1216mc00032")

include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
