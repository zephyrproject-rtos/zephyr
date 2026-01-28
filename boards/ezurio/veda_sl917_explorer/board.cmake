# Copyright (c) 2026 Ezurio
# SPDX-License-Identifier: Apache-2.0

board_runner_args(silabs_commander "--device=SiWG917Y111MGABA")
include(${ZEPHYR_BASE}/boards/common/silabs_commander.board.cmake)

# It is not possible to load/flash firmware using JLink, but it is possible to
# debug with:
#    west attach -r jlink
# Once started, it should be possible to reset the device with "monitor reset"
board_runner_args(jlink "--device=Si917" "--speed=10000")
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)

include_directories(${CMAKE_CURRENT_LIST_DIR}/include)
