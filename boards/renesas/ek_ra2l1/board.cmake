# Copyright (c) 2021-2024 MUNIC SA
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=r7fa2l1ab")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
