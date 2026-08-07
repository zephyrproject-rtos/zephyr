# Copyright (c) 2025 Ambiq Micro Inc.
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=AP510NFA-CBR" "--iface=swd" "--speed=1000")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
