#
# Copyright (c) 2020 Lemonbeat GmbH
#
# SPDX-License-Identifier: Apache-2.0
#

board_runner_args(linkserver  "--device=LPC55S28:LPCXpresso55S28")
board_runner_args(pyocd "--target=lpc55s28")
board_runner_args(jlink "--device=LPC55S28" "--reset-after-load")

include(${ZEPHYR_BASE}/boards/common/linkserver.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
