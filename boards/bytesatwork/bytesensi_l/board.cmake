# Copyright (c) 2024 bytesatwork AG
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=nRF52832_xxAA" "--speed=4000")
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
