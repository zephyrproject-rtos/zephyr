# SPDX-License-Identifier: Apache-2.0

# During gdb session, by default connect to CM4 core.
board_runner_args(openocd "--gdb-init=disconnect")
board_runner_args(openocd "--gdb-init=target extended-remote :3334")
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)

board_runner_args(pyocd "--target=cy8c6xxa")
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
