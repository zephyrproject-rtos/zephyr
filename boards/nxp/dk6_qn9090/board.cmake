#
# Copyright (c) 2023 Sendrato
#
# SPDX-License-Identifier: Apache-2.0
#

board_runner_args(jlink "--device=QN9090")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
