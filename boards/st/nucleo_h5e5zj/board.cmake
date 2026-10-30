# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw")
board_runner_args(stlink_gdbserver "--apid=1")

include("${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake")
include("${ZEPHYR_BASE}/boards/common/stlink_gdbserver.board.cmake")
