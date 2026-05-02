# Copyright (c) 2024 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=R9A08G045S33_M33_0" "--speed=15000")
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
