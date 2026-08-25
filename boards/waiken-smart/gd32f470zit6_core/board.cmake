# Copyright (c) 2026 Zhijian Han
# SPDX-License-Identifier: Apache-2.0

# GD32F470xx series is not yet in the SEGGER J-Link device database.
# Use the closest GD32F450 device as a stand-in (same Cortex-M4F core,
# flash base 0x08000000, SRAM base 0x20000000 and identical CPU TAP ID).
board_runner_args(jlink "--device=GD32F450ZK" "--speed=4000")

# Make J-Link the default runner, since it is the most common probe used with
# this board. OpenOCD (CMSIS-DAP) remains available via --runner openocd.
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
