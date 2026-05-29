# Copyright (c) 2024 GARDENA GmbH
#
# SPDX-License-Identifier: Apache-2.0

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)

board_runner_args(jlink "--device=SiM3U167")
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
