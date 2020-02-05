#
# Copyright (c) 2019, MADMACHINE LIMITED
#
# SPDX-License-Identifier: Apache-2.0
#

board_set_debugger_ifnset(pyocd)
board_set_flasher_ifnset(pyocd)

board_runner_args(pyocd "--target=mimxrt1050_quadspi")
board_runner_args(pyocd "--tool-opt=--script=${ZEPHYR_BASE}/boards/arm/mm_swiftio/burner/pyocd_user.py")

include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
