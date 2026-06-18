# Copyright (c) 2024-2025 Analog Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=MAX32690" "--reset-after-load" "--reset-type=12")

# Force MAX32 openocd configuration to use M4 core, since RV32 debug pins
# aren't exposed
set(MAX32_TARGET_CFG "${CONFIG_SOC}.cfg")
set(MAX32_INTERFACE_CFG "cmsis-dap.cfg")

include(${ZEPHYR_BASE}/boards/common/openocd-adi-max32.boards.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
