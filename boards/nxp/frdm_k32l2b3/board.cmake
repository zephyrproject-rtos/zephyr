#
# Copyright (c) 2025 Ishraq Ibne Ashraf <ishraq.i.ashraf@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0
#

board_runner_args(linkserver "--device=K32L2B31A:FRDM-K32L2B")
board_runner_args(pyocd "--target=k32l2b3")

include(${ZEPHYR_BASE}/boards/common/linkserver.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
