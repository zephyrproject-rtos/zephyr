#
# Copyright 2022-2023 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

board_runner_args(linkserver  "--device=LPC55S36:LPCXpresso55S36")
board_runner_args(jlink "--device=LPC55S36" "--reset-after-load")
board_runner_args(pyocd "--target=lpc55s36")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/linkserver.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
