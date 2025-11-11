# Copyright (c) 2024 Daikin Comfort Technologies North America, Inc.

# SPDX-License-Identifier: Apache-2.0
#

board_runner_args(jlink "--device=MGM240PB32VNA")
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
