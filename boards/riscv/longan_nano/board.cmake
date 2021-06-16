# Copyright (c) 2021 Tokita, Hiroshi <tokita.hiroshi@gmail.com>
# SPDX-License-Identifier: Apache-2.0

board_runner_args(openocd "--cmd-pre-load=gd32vf103-pre-load")
board_runner_args(openocd "--cmd-load=gd32vf103-load")
board_runner_args(openocd "--cmd-post-verify=gd32vf103-post-verify")

if(BOARD STREQUAL "longan_nano_lite")
board_runner_args(jlink "--device=GD32VF103C8T6")
else()
board_runner_args(jlink "--device=GD32VF103CBT6")
endif()

board_runner_args(jlink "--if=JTAG")
board_runner_args(jlink "--speed=4000")
board_runner_args(jlink "--tool-opt=-jtagconf -1,-1")

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
