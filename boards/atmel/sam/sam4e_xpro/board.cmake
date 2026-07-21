# SPDX-License-Identifier: Apache-2.0

board_runner_args(openocd --cmd-post-verify "at91sam4 gpnvm set 1")
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)

include(${ZEPHYR_BASE}/boards/common/bossac.board.cmake)
