# SPDX-License-Identifier: Apache-2.0

board_runner_args(openocd "--use-elf" "--config=${BOARD_DIR}/support/scobc-a1.cfg")

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
