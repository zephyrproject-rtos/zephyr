#
# Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

board_runner_args(linkserver "--device=LPC54628:LPCXpresso54628")

include(${ZEPHYR_BASE}/boards/common/linkserver.board.cmake)
