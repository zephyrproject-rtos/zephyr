# Copyright (c) 2025 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=R9A07G074M04")
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
