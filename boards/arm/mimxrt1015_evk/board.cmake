#
# Copyright (c) 2019, NXP
#
# SPDX-License-Identifier: Apache-2.0
#

board_runner_args(jlink "--device=MIMXRT1015")
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
