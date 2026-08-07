# Copyright (c) 2024 Ian Morris
# SPDX-License-Identifier: Apache-2.0

board_runner_args(pyocd "--target=r7fa4m1ab")

include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
