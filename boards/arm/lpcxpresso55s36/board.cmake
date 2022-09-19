#
# Copyright 2022 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

board_runner_args(jlink "--device=LPC55S36" "--reset-after-load")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
