#
# Copyright (c) 2018, Christian Taedcke
# Copyright (c) 2022 T-Mobile USA, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#

board_runner_args(jlink "--device=EFM32PG12BxxxF1024")
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
