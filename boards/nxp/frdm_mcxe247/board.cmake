#
# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

board_runner_args(linkserver "--device=MCXE247:FRDM-MCXE247")
board_runner_args(jlink "--device=MCXE247")

include(${ZEPHYR_BASE}/boards/common/linkserver.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
