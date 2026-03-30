# Copyright (c) 2026 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=R9A09G087M44_R52_0")
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
