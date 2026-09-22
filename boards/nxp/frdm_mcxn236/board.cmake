#
# Copyright 2024 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

board_runner_args(jlink "--device=MCXN236" "--reset-after-load")
board_runner_args(linkserver  "--device=MCXN236:FRDM-MCXN236")

# Pyocd support added with the NXP.MCXN236_DFP.17.0.0.pack CMSIS Pack
board_runner_args(pyocd "--target=mcxn236")

include(${ZEPHYR_BASE}/boards/common/linkserver.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
