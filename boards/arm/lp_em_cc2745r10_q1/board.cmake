# Copyright (c) 2025 Texas Instruments Incorporated
# Copyright (c) 2024 BayLibre, SAS
#
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=CC2745R10-Q1")
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
