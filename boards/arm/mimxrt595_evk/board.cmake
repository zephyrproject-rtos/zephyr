#
# Copyright (c) 2022, NXP
#
# SPDX-License-Identifier: Apache-2.0
#

board_runner_args(jlink "--device=MIMXRT595S_M33" "--reset-after-load")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
