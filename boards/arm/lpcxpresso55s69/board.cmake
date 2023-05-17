#
# Copyright 2019, 2023 NXP
#
# SPDX-License-Identifier: Apache-2.0
#


## DAP Link implementation in pyocd is underway,
## until then jlink can be used or copy image to storage

if(CONFIG_BOARD_LPCXPRESSO55S69_CPU0 OR CONFIG_SECOND_CORE_MCUX)
board_runner_args(jlink "--device=LPC55S69_M33_0")
board_runner_args(linkserver  "--device=LPC55S69:LPCXpresso55S69")
board_runner_args(linkserver  "--override=/device/memory/0/flash-driver=LPC55xx_S.cfx")
board_runner_args(linkserver  "--override=/device/memory/0/location=0x10000000")
elseif(CONFIG_BOARD_LPCXPRESSO55S69_CPU1)
board_runner_args(jlink "--device=LPC55S69_M33_1")
endif()

board_runner_args(pyocd "--target=lpc55s69")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/linkserver.board.cmake)
