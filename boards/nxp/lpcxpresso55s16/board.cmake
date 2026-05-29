#
# Copyright (c) 2020 Henrik Brix Andersen <henrik@brixandersen.dk>
# Copyright 2023 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

board_runner_args(linkserver  "--device=LPC55S16:LPCXpresso55S16")
board_runner_args(jlink "--device=LPC55S16" "--reset-after-load")
board_runner_args(pyocd "--target=lpc55s16")

include(${ZEPHYR_BASE}/boards/common/linkserver.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
