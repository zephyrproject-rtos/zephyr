# Copyright (c) 2024 Antmicro <www.antmicro.com>
# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS renode)
set(RENODE_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/support/myra_sip_baseboard.resc)
set(RENODE_UART sysbus.lpuart1)

board_runner_args(openocd "--config=${BOARD_DIR}/support/openocd_myra_sip_baseboard.cfg")

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
