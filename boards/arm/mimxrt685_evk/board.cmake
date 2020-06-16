#
# Copyright (c) 2020, NXP
#
# SPDX-License-Identifier: Apache-2.0
#

board_runner_args(jlink "--device=MIMXRT685S_M33")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
