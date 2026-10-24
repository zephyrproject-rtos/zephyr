# Copyright (c) 2026 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

# "SIMG301M104LIL" is an alias Silicon Labs' own JLinkDevices.xml (shipped with
# Simplicity Studio / their Matter SDK provisioning tools, not stock SEGGER
# J-Link installs as of this writing) defines for the generic "SIxx301" entry.
# Merge that XML into the local J-Link install before this device name resolves.
board_runner_args(jlink "--device=SIMG301M104LIL")
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)

board_runner_args(silabs_commander "--device=${CONFIG_SOC}")
include(${ZEPHYR_BASE}/boards/common/silabs_commander.board.cmake)
