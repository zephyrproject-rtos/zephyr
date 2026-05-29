# SPDX-FileCopyrightText: Copyright (c) 2026 Navimatix GmbH
# SPDX-FileCopyrightText: Copyright (c) 2026 TiaC Systems
# SPDX-License-Identifier: Apache-2.0

board_runner_args(dfu-util "--pid=0483:df11" "--alt=0" "--dfuse")
board_runner_args(openocd "--config=${BOARD_DIR}/support/openocd.cfg")
board_runner_args(jlink "--device=STM32F072C8")

include(${ZEPHYR_BASE}/boards/common/dfu-util.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
