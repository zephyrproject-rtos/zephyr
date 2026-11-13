#
# Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

board_runner_args(linkserver "--device=LPC54628:LPCXpresso54628")
board_runner_args(jlink "--device=LPC54628J512")

include(${ZEPHYR_BASE}/boards/common/linkserver.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
