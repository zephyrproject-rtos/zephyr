# Copyright (c) 2026 Aerlync Labs Inc.
#
# SPDX-License-Identifier: Apache-2.0

board_runner_args(pyocd "--target=lpc845")

include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
