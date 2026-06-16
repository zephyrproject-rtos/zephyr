# Copyright (c) 2026 Oleksii Mamontov <mamontov1@gmail.com>
# SPDX-License-Identifier: Apache-2.0

board_runner_args(openocd "--config=${BOARD_DIR}/support/openocd.cfg")

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
