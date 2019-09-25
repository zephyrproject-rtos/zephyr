# SPDX-License-Identifier: Apache-2.0
board_runner_args(pyocd "--target=stm32g071rb")

include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
