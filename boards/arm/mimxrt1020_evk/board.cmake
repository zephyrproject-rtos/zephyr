#
# Copyright (c) 2018, NXP
#
# SPDX-License-Identifier: Apache-2.0
#

board_runner_args(jlink "--device=MCIMXRT1021")
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
