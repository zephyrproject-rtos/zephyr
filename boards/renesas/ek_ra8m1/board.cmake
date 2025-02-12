# Copyright (c) 2024 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=R7FA8M1AH")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
