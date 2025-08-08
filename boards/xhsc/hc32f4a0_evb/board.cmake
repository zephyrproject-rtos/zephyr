# Copyright (C) 2024-2025, Xiaohua Semiconductor Co., Ltd.
# SPDX-License-Identifier: Apache-2.0

board_runner_args(pyocd "--target=HC32F4A0" "--frequency=10000000")
board_runner_args(jlink "--device=HC32F4A0" "--speed=4000")

include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
