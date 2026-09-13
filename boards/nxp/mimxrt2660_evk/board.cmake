#
# SPDX-FileCopyrightText: Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

board_runner_args(jlink "--device=MIMXRT2663xxxxx_M85")

# LinkServer is not supported for this board yet.
# board_runner_args(linkserver "--device=MIMXRT2663:MIMXRT2660-EVK")

# include(${ZEPHYR_BASE}/boards/common/linkserver.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
