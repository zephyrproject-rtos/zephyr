# Copyright (c) 2018 Henrik Brix Andersen <henrik@brixandersen.dk>
#
# SPDX-License-Identifier: Apache-2.0

board_runner_args(bossac "--offset=0x2000")

include(${ZEPHYR_BASE}/boards/common/bossac.board.cmake)
