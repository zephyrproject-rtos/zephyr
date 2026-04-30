# Copyright (c) 2026, Realtek Semiconductor Corporation.
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=RTL8752H" "--speed=4000")
board_runner_args(pyocd "--target=RTL8752H" "--tool-opt=--pack=${ZEPHYR_HAL_REALTEK_MODULE_DIR}/bee/cmsis_pack/Realtek.RTL8752H_DFP.1.0.1.pack")

include(${ZEPHYR_BASE}/boards/common/mpcli.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
