#
# Copyright 2024 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

board_runner_args(jlink "--device=MCXN236" "--reset-after-load")
board_runner_args(linkserver  "--device=MCXN236:FRDM-MCXN236")
board_runner_args(linkserver  "--core=cm33_core0")
board_runner_args(linkserver  "--override=/device/memory/1/flash-driver=MCXNxxx_S.cfx")
board_runner_args(linkserver  "--override=/device/memory/1/location=0x10000000")
# Linkserver v1.4.85 and earlier do not include the secure regions in the
# MCXN236 memory map, so we add them here
board_runner_args(linkserver "--override=/device/memory/-=\{\"location\":\"0x30000000\",\
                               \"size\":\"0x00040000\",\"type\":\"RAM\"\}")
# Define region for peripherals
board_runner_args(linkserver "--override=/device/memory/-=\{\"location\":\"0x50000000\",\
                               \"size\":\"0x00140000\",\"type\":\"RAM\"\}")

# Pyocd support added with the NXP.MCXN236_DFP.17.0.0.pack CMSIS Pack
board_runner_args(pyocd "--target=mcxn236")

include(${ZEPHYR_BASE}/boards/common/linkserver.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
