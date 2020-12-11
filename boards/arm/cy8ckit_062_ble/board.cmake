#
# Copyright (c) 2018, Cypress
# Copyright (c) 2020, ATL Electronics
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_BOARD_CY8CKIT_062_BLE_M0)
board_runner_args(jlink "--device=CY8C6xx7_CM0p" "--speed=2000")
endif()
if(CONFIG_BOARD_CY8CKIT_062_BLE_M4)
board_runner_args(jlink "--device=CY8C6xx7_CM4" "--speed=2000")
endif()

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
