# SPDX-License-Identifier: Apache-2.0

board_runner_args(openocd --cmd-load "npcx_write_image")
board_runner_args(openocd --cmd-verify "npcx_verify_image")

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
