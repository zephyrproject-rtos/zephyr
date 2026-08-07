# Copyright 2022-2023 NXP
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=RW612" "--reset-after-load")

board_runner_args(linkserver  "--device=RW612:FRDM-RW612")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/linkserver.board.cmake)
