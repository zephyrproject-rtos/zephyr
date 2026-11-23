# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# The board ships with a UF2 bootloader, entered by double-tapping the reset
# button, so the UF2 runner is the default.
if(CONFIG_BOARD_XIAO_NRF52840_PLUS_NRF52840)
  board_runner_args(uf2 "--board-id=nRF52840-SeeedXiao-v1")
endif()
include(${ZEPHYR_BASE}/boards/common/uf2.board.cmake)

board_runner_args(nrfjprog "--nrf-family=NRF52")
board_runner_args(jlink "--device=nRF52840_xxAA" "--speed=4000")
board_runner_args(pyocd "--target=nrf52840" "--frequency=4000000")
include(${ZEPHYR_BASE}/boards/common/nrfutil.board.cmake)
include(${ZEPHYR_BASE}/boards/common/nrfjprog.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/blackmagicprobe.board.cmake)
