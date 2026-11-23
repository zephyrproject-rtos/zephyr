# Copyright (c) 2026 GigaDevice Semiconductor Inc.
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=GD32F527IS" "--speed=4000")

# The DFP pack is not part of the HAL module, copy it to the path below
# before selecting the pyocd runner.
board_runner_args(pyocd "--target=GD32F527IS" "--frequency=4000000" "--tool-opt=--pack=${ZEPHYR_HAL_GIGADEVICE_MODULE_DIR}/${CONFIG_SOC_SERIES}/support/GigaDevice.GD32F527_DFP.1.4.0.pack")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
