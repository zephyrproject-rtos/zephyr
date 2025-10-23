# Copyright (c) 2025 Fabian Pflug
#
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=CC2340R5" "--iface=swd")
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
