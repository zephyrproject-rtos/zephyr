#
# Copyright (c) 2022 metraTec
# Copyright 2023 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

board_runner_args(linkserver  "--device=LPC55S06:LPCXpresso55S06")
board_runner_args(jlink "--device=LPC55S06" "--reset-after-load")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/linkserver.board.cmake)
