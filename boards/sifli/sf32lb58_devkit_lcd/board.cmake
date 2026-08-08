# Copyright (c) 2026 Qingsong Gou <gouqs@hotmail.com>
# SPDX-License-Identifier: Apache-2.0

# keep first
board_runner_args(sftool "--chip=SF32LB58")

# keep first
include(${ZEPHYR_BASE}/boards/common/sftool.board.cmake)
