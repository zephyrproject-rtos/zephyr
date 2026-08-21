# SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
# SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

board_set_debugger_ifnset(jlink)
board_set_flasher_ifnset(jlink)

board_runner_args(openocd "--target-handle=TARGET.cm33")
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)

# Generate J-Link flash script dynamically in the build directory
configure_file(
  ${CMAKE_CURRENT_LIST_DIR}/support/flash.jlink.in
  ${PROJECT_BINARY_DIR}/flash.jlink
  @ONLY
)

board_runner_args(jlink "--device=PSC3xxF")
board_runner_args(jlink "--speed=auto")
board_runner_args(jlink "--iface=swd")
board_runner_args(jlink "--flash-script=${PROJECT_BINARY_DIR}/flash.jlink")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
