# Copyright (c) 2024 Michael Hope
# SPDX-License-Identifier: Apache-2.0

board_runner_args(minichlink "--dt-flash=y")
include(${ZEPHYR_BASE}/boards/common/minichlink.board.cmake)
