#
# Copyright (c) 2022 metraTec
#
# SPDX-License-Identifier: Apache-2.0
#

board_runner_args(jlink "--device=LPC55S06" "--reset-after-load")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
