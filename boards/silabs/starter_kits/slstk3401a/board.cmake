# Copyright (c) 2020, Rafael Dias Menezes <rdmeneze@gmail.com>
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=EFM32PG1BxxxF256")
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
