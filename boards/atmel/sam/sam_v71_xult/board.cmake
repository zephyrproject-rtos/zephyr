# SPDX-License-Identifier: Apache-2.0

board_runner_args(openocd --cmd-post-verify "atsamv gpnvm set 1")
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
