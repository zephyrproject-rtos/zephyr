# Copyright (c) 2022 YuLong Yao <feilongphone@gmail.com>
# SPDX-License-Identifier: Apache-2.0

board_runner_args(pyocd "--target=gd32a503vd" "--tool-opt=--pack=${ZEPHYR_HAL_GIGADEVICE_MODULE_DIR}/${CONFIG_SOC_SERIES}/support/GigaDevice.GD32A50x_DFP.1.0.0.pack")
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)

board_runner_args(gd32isp "--device=GD32A503VDT3")
include(${ZEPHYR_BASE}/boards/common/gd32isp.board.cmake)
