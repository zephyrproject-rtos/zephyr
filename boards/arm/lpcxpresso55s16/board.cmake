#
# Copyright (c) 2020 Henrik Brix Andersen <henrik@brixandersen.dk>
#
# SPDX-License-Identifier: Apache-2.0
#

board_runner_args(jlink "--device=LPC55S16" "--reset-after-load")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
