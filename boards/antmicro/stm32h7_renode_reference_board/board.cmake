# Copyright (c) 2026 Antmicro <www.antmicro.com>
# SPDX-License-Identifier: Apache-2.0

board_runner_args(openocd "--config=${BOARD_DIR}/support/stm32h7xx_over_ft4232h-jtag.cfg")

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)

set(SUPPORTED_EMU_PLATFORMS renode)
set(RENODE_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/support/stm32h7_renode_reference_board.resc)
set(RENODE_UART sysbus.usart2)

set_ifndef(BOARD_SIM_RUNNER renode)
set_ifndef(BOARD_ROBOT_RUNNER renode-robot)
include(${ZEPHYR_BASE}/boards/common/renode.board.cmake)
include(${ZEPHYR_BASE}/boards/common/renode_robot.board.cmake)
