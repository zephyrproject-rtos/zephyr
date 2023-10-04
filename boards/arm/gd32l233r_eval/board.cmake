# Copyright (c) 2022 BrainCo Inc.
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=GD32L233RCT6" "--iface=SWD" "--speed=4000")
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
