#
# Copyright 2024 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

board_runner_args(jlink "--device=MCXC242")
board_runner_args(linkserver "--device=MCXC242:FRDM-MCXC242")
board_runner_args(pyocd "--target=mcxc242")

include(${ZEPHYR_BASE}/boards/common/linkserver.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
