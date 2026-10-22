# Copyright (c) 2025 Core Devices LLC
# SPDX-License-Identifier: Apache-2.0

board_runner_args(sftool "--chip=SF32LB52")

include(${ZEPHYR_BASE}/boards/common/sftool.board.cmake)
