#
# Copyright (c) 2017, NXP
#
# SPDX-License-Identifier: Apache-2.0
#

board_runner_args(jlink "--device=Cortex-M7")
include($ENV{ZEPHYR_BASE}/boards/common/jlink.board.cmake)
