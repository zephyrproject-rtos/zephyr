# SPDX-FileCopyrightText: 2026 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=R7KA6B1BG")
board_runner_args(jlink "--tool-opt=-JLinkScriptFile ${PROJECT_BINARY_DIR}/ra6b1.jlinkscript")

board_runner_args(rfp "--device=RA6B1")
board_runner_args(rfp "--interface=uart")

# keep first
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
