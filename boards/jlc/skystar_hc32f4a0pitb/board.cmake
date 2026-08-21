# Copyright (c) 2026 Liu Changjie <liucj1228@outlook.com>
# SPDX-License-Identifier: Apache-2.0

board_runner_args(pyocd "--target=hc32f4a0pitb" "--frequency=4000000")

include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
