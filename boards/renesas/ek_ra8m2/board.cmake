# Copyright (c) 2025 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_R7KA8M2JFLCAC_CM85)
 board_runner_args(jlink "--device=R7KA8M2JF_CPU0" "--reset-after-load")
endif()

board_runner_args(pyocd "--target=R7KA8M2JF")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
