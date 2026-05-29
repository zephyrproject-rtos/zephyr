# Copyright (c) 2026 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_R7KA8T2LFLCAC_CM85)
 board_runner_args(jlink "--device=R7KA8T2LF_CPU0")
else()
 board_runner_args(jlink "--device=R7KA8T2LF_CPU1")
endif()

board_runner_args(pyocd "--target=R7KA8T2LF")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
