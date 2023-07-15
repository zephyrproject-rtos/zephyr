# Copyright (c) 2023 David Ullmann
# spdx-license-identifier: apache-2.0

board_runner_args(pyocd "--target=cy8c6xxa")
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
