#
# Copyright (c) 2019, NXP
#
# SPDX-License-Identifier: Apache-2.0
#


## DAP Link implementation in pyocd is underway,
## until then jlink can be used or copy image to storage

if(CONFIG_BOARD_LPCXPRESSO55S69_CPU0)
board_runner_args(jlink "--device=LPC55S69_core0")
endif()
if(CONFIG_BOARD_LPCXPRESSO55S69_CPU1)
board_runner_args(jlink "--device=LPC55S69_core1")
endif()

board_runner_args(pyocd "--target=lpc55s69")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)

set(TFM_TARGET_PLATFORM "LPC55S69")
