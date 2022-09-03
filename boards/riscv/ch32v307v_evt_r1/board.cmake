# Copyright (c) 2022 TOKITA Hiroshi <tokita.hiroshi@gmail.com>
# SPDX-License-Identifier: Apache-2.0

# It seems the 'reset run' command make flash failure.
# Call 'reset halt', 'resume', and 'shutdown' before
# 'reset run' to workarounds.
board_runner_args(openocd "--cmd-post-verify=ch32v307v-shutdown")

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
