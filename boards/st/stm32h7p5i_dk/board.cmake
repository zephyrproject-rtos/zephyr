# SPDX-FileCopyrightText: Copyright (c) 2026 STMicroelectronics
# SPDX-License-Identifier: Apache-2.0

set(extloader "--extload=MX25LM51245G_STM32H7P5I-DK.stldr")

board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw" "${extloader}")
board_runner_args(stlink_gdbserver "--apid=1" "${extloader}")

include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
include(${ZEPHYR_BASE}/boards/common/stlink_gdbserver.board.cmake)
