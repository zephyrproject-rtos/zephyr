#
# Copyright 2023 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

board_runner_args(jlink "--device=MIMXRT1042XXX6B")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
