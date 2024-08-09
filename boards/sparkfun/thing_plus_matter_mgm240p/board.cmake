# Copyright (c) 2024 Daikin Comfort Technologies North America, Inc.

# SPDX-License-Identifier: Apache-2.0
#

board_runner_args(jlink "--device=EFR32MG24BxxxF1536")
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
