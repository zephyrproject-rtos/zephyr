# SPDX-License-Identifier: Apache-2.0

# keep first
board_runner_args(pyocd "--target=R7FA8D1BH")

# keep first
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
