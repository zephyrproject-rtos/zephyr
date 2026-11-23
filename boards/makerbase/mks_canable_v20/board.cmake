# Copyright (c) 2024 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
# SPDX-License-Identifier: Apache-2.0

board_runner_args(openocd "--config=${BOARD_DIR}/support/openocd.cfg")

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
