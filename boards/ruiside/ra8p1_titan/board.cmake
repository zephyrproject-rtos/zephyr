# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_R7KA8P1KFLCAC_CM85)
 board_runner_args(jlink "--device=R7KA8P1KF_CPU0" "--reset-after-load")
else()
 board_runner_args(jlink "--device=R7KA8P1KF_CPU1")
endif()

board_runner_args(pyocd "--target=R7KA8P1KF")

include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
