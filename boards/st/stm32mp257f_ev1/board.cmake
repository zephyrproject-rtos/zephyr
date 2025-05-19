# Copyright (C) 2025 Savoir-faire Linux, Inc.
# SPDX-License-Identifier: Apache-2.0

board_runner_args(openocd "--config=${BOARD_DIR}/support/openocd.cfg")
board_runner_args(openocd "--gdb-init=target extended-remote :3334")

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
