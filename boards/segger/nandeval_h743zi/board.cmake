# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=STM32H743ZI" "--speed=4000")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
