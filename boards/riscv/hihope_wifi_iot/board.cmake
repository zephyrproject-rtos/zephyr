# Copyright (c) 2023 Chen Xingyu <hi@xingrz.me>
# SPDX-License-Identifier: Apache-2.0

# This board shipped with a CH340G USB-to-serial chip, which is okay to operate
# at 3M baudrate.
board_runner_args(hiburn "--baudrate=3000000")

include(${ZEPHYR_BASE}/boards/common/hiburn.board.cmake)
