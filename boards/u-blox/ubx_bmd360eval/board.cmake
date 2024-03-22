# BMD-360-EVAL board configuration

# Copyright (c) 2021 u-blox AG
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=nRF52811_xxAA" "--speed=4000")
include(${ZEPHYR_BASE}/boards/common/nrfjprog.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd-nrf5.board.cmake)
